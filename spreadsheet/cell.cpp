#include "cell.h"
#include "sheet.h"

#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <optional>

class Cell::Impl {
public:
    virtual ~Impl() = default;
    virtual Value GetValue() const = 0;
    virtual std::string GetText() const = 0;
    virtual std::vector<Position> GetDependencies() const {
        return {};
    }
    virtual bool HasCache() const { return false; }
    virtual void WipeCache() const {}
};

class Cell::EmptyImpl : public Impl {
public:
    Value GetValue() const override {
        return 0.0;
    }
    std::string GetText() const override {
        return {};
    }
};

class Cell::TextImpl : public Impl {
public:
    TextImpl(std::string text)
        : text_(text) {
    }

    Value GetValue() const override {
        if (text_[0] != ESCAPE_SIGN) {
            return GetText();
        } else {
            return text_.substr(1);
        }
    }

    std::string GetText() const override {
        return text_;
    }

private:
     std::string text_;
};

class Cell::FormulaImpl : public Impl {
public:
    FormulaImpl(std::string formula, Sheet& sheet)
        : formula_(ParseFormula(formula)), sheet_(sheet) {
    }

    Value GetValue() const override {
        auto formula_value = formula_->Evaluate(sheet_);
        if (std::holds_alternative<double>(formula_value)) {
            return std::get<double>(formula_value);
        } else {
            return std::get<FormulaError>(formula_value);
        }
    }

    std::string GetText() const override {
        std::string formula_text = "=" + formula_->GetExpression();
        return formula_text;
    }

    std::vector<Position> GetDependencies() const override {
        return formula_->GetReferencedCells();
    }

    bool HasCache() const override {
        return cache_.has_value();
    }

    void WipeCache() const override {
        cache_.reset();
    }

private:
    std::unique_ptr<FormulaInterface> formula_;
    mutable std::optional<FormulaInterface::Value> cache_;
    const Sheet& sheet_;
};

Cell::Cell(Sheet& sheet) 
    : impl_(std::make_unique<EmptyImpl>()), sheet_(sheet) {
}

Cell::~Cell() = default;

void Cell::Set(std::string text) {
    if (text.empty()) {
      impl_ = std::make_unique<EmptyImpl>();   
    } else if (text[0] == FORMULA_SIGN && text.size() > 1) {
        std::unique_ptr<Impl> impl_temp = std::make_unique<FormulaImpl>(text.substr(1), sheet_);
        if (WillCauseCircularDependency(*impl_temp)) {
            throw CircularDependencyException("Circular dependency");
        }        
        impl_ = std::move(impl_temp);
    } else {
        impl_ = std::make_unique<TextImpl>(text);
    }        
    for (auto cell_ptr : precedent_cells) {
        cell_ptr->dependent_cells.erase(this);
    }
    precedent_cells.clear();
    for (auto pos : impl_->GetDependencies()) {
        auto cell_ptr = sheet_.GetConcreteCell(pos);
        if (cell_ptr == nullptr) {
            sheet_.SetCell(pos, "");
            cell_ptr = sheet_.GetConcreteCell(pos);            
        }
        precedent_cells.insert(cell_ptr);
        cell_ptr->dependent_cells.insert(this);
    }
    InvalidateCache();
}

void Cell::Clear() {
    impl_ = std::make_unique<EmptyImpl>();
}

Cell::Value Cell::GetValue() const {
    return impl_->GetValue();
}

std::string Cell::GetText() const {
    return impl_->GetText(); 
}

std::vector<Position> Cell::GetReferencedCells() const {
    return impl_->GetDependencies();
}

bool Cell::HasDependentCells() const {
    return !dependent_cells.empty();
}

bool FindCircularByDFS(const Cell* cell_ptr, std::unordered_set<const Cell*>& path, 
                                             std::unordered_set<const Cell*>& visited) {
    if (path.find(cell_ptr) != path.end()) return true;
    if (visited.find(cell_ptr) != visited.end()) return false;
    path.insert(cell_ptr);
    visited.insert(cell_ptr);
    for (auto child_ptr : cell_ptr->GetPrecedentCellsPtrs()) {
        if (FindCircularByDFS(child_ptr, path, visited)) {
            return true;
        }
    }
    path.erase(cell_ptr);
    return false;
}

bool Cell::WillCauseCircularDependency(const Impl& impl_temp) const {
    std::unordered_set<const Cell*> visited;
    std::unordered_set<const Cell*> path;
    path.insert(static_cast<const Cell*>(this));
    for (auto pos : impl_temp.GetDependencies()) {
        if (FindCircularByDFS(static_cast<const Cell*>(sheet_.GetConcreteCell(pos)), path, visited)) {
            return true;
        }
    }
    return false;
}

void Cell::InvalidateCache() {
    if (dependent_cells.empty()) return;
    for (auto child_ptr : dependent_cells) {
        if (child_ptr->impl_->HasCache()) {
            child_ptr->impl_->WipeCache();
            child_ptr->InvalidateCache();
        }
    }
}
