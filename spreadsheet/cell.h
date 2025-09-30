#pragma once

#include "common.h"
#include "formula.h"

#include <functional>
#include <unordered_set>

class Sheet;

class Cell : public CellInterface {
public:
    Cell(Sheet& sheet);
    ~Cell();

    void Set(std::string text);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;

    std::vector<Position> GetReferencedCells() const override;

    bool HasDependentCells() const;

    std::unordered_set<Cell*> GetPrecedentCellsPtrs() const {
        return precedent_cells;
    } 

private:
    class Impl;
    class EmptyImpl;
    class TextImpl;
    class FormulaImpl;
    std::unique_ptr<Impl> impl_;

    bool WillCauseCircularDependency(const Impl& impl_temp) const;
    void InvalidateCache();

    Sheet& sheet_;
    // Указатели на ячейки, от которых зависит эта ячейка (для отслеживания кольцевых зависимостей)
    std::unordered_set<Cell*> precedent_cells;
    // Указатели на ячейки, которые зависят от этой ячейки (для инвалидации кэша)
    std::unordered_set<Cell*> dependent_cells;
};
