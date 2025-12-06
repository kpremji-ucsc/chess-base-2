#pragma once

#include "Game.h"
#include "Grid.h"
#include "Bitboard.h"
#include "MagicBitboards.h"
#include <vector>
#include <map>

constexpr int pieceSize = 80;

constexpr int WHITE = 1;
constexpr int BLACK = -1;
constexpr int negInfinite = -1000000;
constexpr int posInfinite = 1000000;

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

    void updateAI() override;
    bool gameHasAI() override { return true; }

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
    void generateBishopMoves(int square, std::vector<BitMove>& moves);
    void generateRookMoves(int square, std::vector<BitMove>& moves);
    void generateQueenMoves(int square, std::vector<BitMove>& moves);
    BitboardElement getOccupiedSquares() const;
    BitboardElement getWhitePieces() const;
    BitboardElement getBlackPieces() const;
    bool isSquareOccupied(int square) const;
    bool isSquareOccupiedByColor(int square, bool white) const;

    std::vector<BitMove> getAllLegalMoves(bool forWhite);
    void generateMovesForPiece(int square, std::vector<BitMove>& moves);
    std::vector<BitMove> generateAllMoves(const std::string& state, int playerColor);
    int evaluateBoard(const std::string& state);
    int negamax(std::string& state, int depth, int alpha, int beta, int playerColor);
    BitHolder& getHolderAt(int x, int y);

    static constexpr int e_numBitboards = 13;
    uint64_t _bitboards[e_numBitboards];
    std::map<char, int> _bitboardLookup;
    void buildBitboardsFromState(const std::string& state);
    int _countMoves;
    std::vector<BitMove> _moves;

    Grid* _grid;
};