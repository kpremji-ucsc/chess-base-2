#pragma once

#include "Game.h"
#include "Grid.h"
#include "Bitboard.h"
#include <vector>

constexpr int pieceSize = 80;

class Chess : public Game
{
public:
    Chess();
    ~Chess();

    void setUpBoard() override;

    bool canBitMoveFrom(Bit &bit, BitHolder &src) override;
    bool canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst) override;
    bool actionForEmptyHolder(BitHolder &holder) override;
    void bitMovedFromTo(Bit &bit, BitHolder &src, BitHolder &dst) override;

    void stopGame() override;

    Player *checkForWinner() override;
    bool checkForDraw() override;

    std::string initialStateString() override;
    std::string stateString() override;
    void setStateString(const std::string &s) override;

    Grid* getGrid() override { return _grid; }

private:
    Bit* PieceForPlayer(const int playerNumber, ChessPiece piece);
    Player* ownerAt(int x, int y) const;
    void FENtoBoard(const std::string& fen);
    char pieceNotation(int x, int y) const;

    ChessPiece getPieceType(const Bit& bit) const;
    bool isWhitePiece(const Bit& bit) const;
    void generatePawnMoves(int square, bool isWhite, std::vector<BitMove>& moves);
    void generateKnightMoves(int square, std::vector<BitMove>& moves);
    void generateKingMoves(int square, std::vector<BitMove>& moves);
    BitboardElement getOccupiedSquares() const;
    BitboardElement getWhitePieces() const;
    BitboardElement getBlackPieces() const;
    bool isSquareOccupied(int square) const;
    bool isSquareOccupiedByColor(int square, bool white) const;

    Grid* _grid;
};