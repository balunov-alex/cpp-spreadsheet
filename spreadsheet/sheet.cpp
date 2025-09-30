#include "sheet.h"
#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

std::ostream& operator<<(std::ostream& os, const CellInterface::Value& value) {
    if (std::holds_alternative<std::string>(value)) {
        os << std::get<std::string>(value);
    } else if (std::holds_alternative<double>(value)) {
        os << std::get<double>(value);
    } else {
        os << std::get<FormulaError>(value);
    }
    return os;
}

void Sheet::SetCell(Position pos, std::string text) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position");
    }
    if (table_.size() <= static_cast<size_t>(pos.row)) {
        table_.resize(pos.row + 1);
        if (table_[0].size() > static_cast<size_t>(pos.col)) {
            for (auto it = table_.end() - 1; it->size() != table_[0].size(); --it) {
                it->resize(table_[0].size());
            }
        }
    }
    if (table_[0].size() <= static_cast<size_t>(pos.col)) {
        for (auto& row_vec : table_) {
            row_vec.resize(pos.col + 1);
        }
    }
    if (table_[pos.row][pos.col] == nullptr) {
        table_[pos.row][pos.col] = std::make_unique<Cell>(*this);                
    }
    table_[pos.row][pos.col]->Set(text);
}

const CellInterface* Sheet::GetCell(Position pos) const {
    return GetConcreteCell(pos);
}

CellInterface* Sheet::GetCell(Position pos) {
    return GetConcreteCell(pos);
}

void Sheet::ClearCell(Position pos) {
    auto cell_ptr = GetConcreteCell(pos);
    if (cell_ptr == nullptr) return;
    if (cell_ptr->HasDependentCells()) {
        table_[pos.row][pos.col]->Clear();
    } else {
        table_[pos.row][pos.col] = nullptr;
    }    
}

Size Sheet::GetPrintableSize() const {
    if (table_.empty()) {
        return {0, 0};
    }
    int row_count = 0;
    for (int i = table_.size() - 1; i >= 0; --i) {
        auto first_non_empty_cell_pos = 
        std::find_if(table_[i].begin(), table_[i].end(), [](const auto& ptr_cell) { 
                                                             return ptr_cell != nullptr && !ptr_cell->GetText().empty(); 
                                                         });
        if (first_non_empty_cell_pos != table_[i].end()) {
            row_count = i + 1;
            break;
        }
        if (i == 0) return {0, 0};
    }
    int col_count = 0;
    for (int i = table_[0].size() - 1; i >= 0; --i) {
        for (const auto& row : table_) {
            if (row[i] != nullptr && !row[i]->GetText().empty()) {
                col_count = i + 1;
                break;
            }
        }
        if (col_count != 0) break;             
    }
    return {row_count, col_count};
}

void Sheet::PrintValues(std::ostream& output) const {
    size_t print_row_count = GetPrintableSize().rows;
    size_t print_col_count = GetPrintableSize().cols;
    for (size_t y = 0; y < print_row_count; ++y) {
        for (size_t x = 0; x < print_col_count; ++x) {
            if (table_[y][x]) {
                output << table_[y][x]->GetValue();
            }
            if (x != print_col_count - 1) {
                output << '\t';
            }
        }
        output << '\n';
    }
}

void Sheet::PrintTexts(std::ostream& output) const {
    size_t print_row_count = GetPrintableSize().rows;
    size_t print_col_count = GetPrintableSize().cols;
    for (size_t y = 0; y < print_row_count; ++y) {
        for (size_t x = 0; x < print_col_count; ++x) {
            if (table_[y][x]) {
                output << table_[y][x]->GetText();
            }
            if (x != print_col_count - 1) {
                output << '\t';
            }
        }
        output << '\n';
    }
}

const Cell* Sheet::GetConcreteCell(Position pos) const {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position");
    }
    if (static_cast<size_t>(pos.row) >= table_.size() || static_cast<size_t>(pos.col) >= table_[0].size()) {
        return nullptr;
    }
    return table_[pos.row][pos.col].get();
}

Cell* Sheet::GetConcreteCell(Position pos) {
    return const_cast<Cell*>(static_cast<const Sheet&>(*this).GetConcreteCell(pos));
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}
