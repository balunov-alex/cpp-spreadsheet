#include "formula.h"
#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

using namespace std::literals;

FormulaError::FormulaError(FormulaError::Category category)
    : category_(category) {
}

FormulaError::Category FormulaError::GetCategory() const {
    return category_;
}

bool FormulaError::operator==(FormulaError rhs) const {
    return category_ == rhs.category_;
}

std::string_view FormulaError::ToString() const {
    switch (category_) {
        case Category::Ref: return "#REF!";
        case Category::Value: return "#VALUE!";
        case Category::Arithmetic: return "#ARITHM!";
        default: return "#UNKNOWN!";
    }
}

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << fe.ToString();
}

namespace {
class Formula : public FormulaInterface {
public:
// Реализуйте следующие методы:
    explicit Formula(std::string expression) try
        : ast_(ParseFormulaAST(expression)) {    
    } catch (const std::exception& exc) {
        throw FormulaException(exc.what());
    }

    Value Evaluate(const SheetInterface& sheet) const override {
        const std::function<double(Position)> get_cells_value_by_pos = [&sheet](Position pos)->double {
            if (!pos.IsValid()) {
                throw FormulaError(FormulaError::Category::Ref);
            }
            auto cell_ptr = sheet.GetCell(pos);
            if (cell_ptr == nullptr) {
                return 0.0;
            }
            const auto cell_value = cell_ptr->GetValue();
            if (std::holds_alternative<double>(cell_value)) {
                return std::get<double>(cell_value);
            }
            if (std::holds_alternative<std::string>(cell_value)) {
                double number = 0; 
                const auto str_to_check = std::get<std::string>(cell_value);
                size_t processed_chars_count = 0;
                try { number = std::stod(str_to_check, &processed_chars_count); } catch (...) {
                    throw FormulaError(FormulaError::Category::Value);
                }
                if (processed_chars_count != str_to_check.size()) {
                    throw FormulaError(FormulaError::Category::Value);
                }
                return number;                   
            }
            throw std::get<FormulaError>(cell_value);
        };
        try {
            return ast_.Execute(get_cells_value_by_pos);
        } catch (const FormulaError& formula_exception) {
            return formula_exception;
        }
    }

    std::string GetExpression() const override {
        std::ostringstream output;
        ast_.PrintFormula(output);
        return output.str();
    }

    std::vector<Position> GetReferencedCells() const override {
        auto& cells = ast_.GetCells();
        std::vector<Position> result(cells.begin(), cells.end());
        auto new_end_it = std::unique(result.begin(), result.end());
        result.erase(new_end_it, result.end());
        return result;
    }  

private:
    FormulaAST ast_;
};
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    return std::make_unique<Formula>(std::move(expression));
}
