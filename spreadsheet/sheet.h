#pragma once

#include "cell.h"
#include "common.h"

#include <vector>
#include <unordered_set>
#include <functional>

struct CellHasher {
    size_t operator()(const Position& pos)const {
        return pos.row * Position::MAX_COLS + pos.col;
    }
};
class Sheet : public SheetInterface {
public:
    ~Sheet();

    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;


private:
    std::unordered_map<Position, std::unique_ptr<Cell>, CellHasher> sheet_;
    Cell* FindOrCreateCell(Position pos);
    Cell* GetConcreteCell(Position pos);
    void InvalidateCacheRecursively(Cell* cell, std::unordered_set<Cell*>& visited) const;
    bool HasCircularDependency(Position pos, const std::vector<Position>& new_refs) const;
    bool DFS(Position current, Position target, std::unordered_set<Position, CellHasher>& visited) const;
};