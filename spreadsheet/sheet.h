#pragma once

#include "cell.h"
#include "common.h"
#include <unordered_map>
#include <vector>
#include <unordered_set>
#include <functional>


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

    Cell* FindOrCreateCell(Position pos);
    Cell* GetConcreteCell(Position pos);
private:
    std::unordered_map<Position, std::unique_ptr<Cell>, CellHasher> sheet_;

    void ValidatePosition(Position pos) const;
};