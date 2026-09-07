#pragma once

#include "common.h"
#include "formula.h"

#include <optional>
#include <unordered_map>
#include <vector>
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
    const std::unordered_set<Cell*>& GetDependents() const;
    std::vector<Position> GetReferencedCells() const override;

    void AddDependent(Cell* dependent);
    void RemoveDependent(Cell* dependent);
    bool IsReferenced() const;
    void InvalidateCache();
private:
    class Impl;
    class EmptyImpl;
    class TextImpl;
    class FormulaImpl;
    std::unique_ptr<Impl> impl_;

    Sheet& sheet_;
    mutable std::optional<Value> cache_;
    std::unordered_set<Cell*> dependents_;
};