#include "Chess.h"
#include <limits>
#include <cmath>

Chess::Chess()
{
    _grid = new Grid(8, 8);
}

Chess::~Chess()
{
    delete _grid;
}

char Chess::pieceNotation(int x, int y) const
{
    const char *wpieces = { "0PNBRQK" };
    const char *bpieces = { "0pnbrqk" };
    Bit *bit = _grid->getSquare(x, y)->bit();
    char notation = '0';
    if (bit) {
        notation = bit->gameTag() < 128 ? wpieces[bit->gameTag()] : bpieces[bit->gameTag()-128];
    }
    return notation;
}

Bit* Chess::PieceForPlayer(const int playerNumber, ChessPiece piece)
{
    const char* pieces[] = { "pawn.png", "knight.png", "bishop.png", "rook.png", "queen.png", "king.png" };

    Bit* bit = new Bit();
    // should possibly be cached from player class?
    const char* pieceName = pieces[piece - 1];
    std::string spritePath = std::string("") + (playerNumber == 0 ? "w_" : "b_") + pieceName;
    bit->LoadTextureFromFile(spritePath.c_str());
    bit->setOwner(getPlayerAt(playerNumber));
    bit->setSize(pieceSize, pieceSize);

    return bit;
}

void Chess::setUpBoard()
{
    setNumberOfPlayers(2);
    _gameOptions.rowX = 8;
    _gameOptions.rowY = 8;

    _grid->initializeChessSquares(pieceSize, "boardsquare.png");
    FENtoBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");

    startGame();
}

void Chess::FENtoBoard(const std::string& fen) {
    std::string piecePlacement = fen;
    size_t spacePos = fen.find(' ');
    if (spacePos != std::string::npos) {
        piecePlacement = fen.substr(0, spacePos);
    }

    int rank = 7;
    int file = 0;

    for (char c : piecePlacement) {
        if (c == '/') {
            rank--;
            file = 0;
        } else if (c >= '1' && c <= '8') {
            file += (c - '0');
        } else {
            ChessPiece pieceType = NoPiece;
            bool isWhite = (c >= 'A' && c <= 'Z');
            char pieceChar = isWhite ? c : (c - 'a' + 'A');

            switch (pieceChar) {
                case 'P': pieceType = Pawn; break;
                case 'N': pieceType = Knight; break;
                case 'B': pieceType = Bishop; break;
                case 'R': pieceType = Rook; break;
                case 'Q': pieceType = Queen; break;
                case 'K': pieceType = King; break;
            }

            if (pieceType != NoPiece && file < 8 && rank >= 0) {
                Bit* piece = PieceForPlayer(isWhite ? 0 : 1, pieceType);
                piece->setGameTag(pieceType + (isWhite ? 0 : 128));
                ChessSquare* square = _grid->getSquare(file, rank);
                square->setBit(piece);
                piece->setPosition(square->getPosition());
            }
            file++;
        }
    }
}

bool Chess::actionForEmptyHolder(BitHolder &holder)
{
    return false;
}

bool Chess::canBitMoveFrom(Bit &bit, BitHolder &src)
{
    // need to implement friendly/unfriendly in bit so for now this hack
    int currentPlayer = getCurrentPlayer()->playerNumber() * 128;
    int pieceColor = bit.gameTag() & 128;
    if (pieceColor == currentPlayer) return true;
    return false;
}

bool Chess::canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst)
{
    ChessSquare* srcSquare = static_cast<ChessSquare*>(&src);
    ChessSquare* dstSquare = static_cast<ChessSquare*>(&dst);

    int srcX = srcSquare->getColumn();
    int srcY = srcSquare->getRow();
    int dstX = dstSquare->getColumn();
    int dstY = dstSquare->getRow();

    int srcSquareIndex = srcY * 8 + srcX;
    int dstSquareIndex = dstY * 8 + dstX;

    if (dstSquare->bit() && dstSquare->bit()->getOwner() == bit.getOwner()) {
        return false;
    }

    ChessPiece pieceType = getPieceType(bit);
    bool isWhite = isWhitePiece(bit);

    std::vector<BitMove> moves;

    switch (pieceType) {
        case Pawn:
            generatePawnMoves(srcSquareIndex, isWhite, moves);
            break;
        case Knight:
            generateKnightMoves(srcSquareIndex, moves);
            break;
        case King:
            generateKingMoves(srcSquareIndex, moves);
            break;
        default:
            return false;
    }

    for (const BitMove& move : moves) {
        if (move.to == dstSquareIndex) {
            return true;
        }
    }

    return false;
}

void Chess::stopGame()
{
    _grid->forEachSquare([](ChessSquare* square, int x, int y) {
        square->destroyBit();
    });
}

Player* Chess::ownerAt(int x, int y) const
{
    if (x < 0 || x >= 8 || y < 0 || y >= 8) {
        return nullptr;
    }

    auto square = _grid->getSquare(x, y);
    if (!square || !square->bit()) {
        return nullptr;
    }
    return square->bit()->getOwner();
}

Player* Chess::checkForWinner()
{
    return nullptr;
}

bool Chess::checkForDraw()
{
    return false;
}

std::string Chess::initialStateString()
{
    return stateString();
}

std::string Chess::stateString()
{
    std::string s;
    s.reserve(64);
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
            s += pieceNotation( x, y );
        }
    );
    return s;}

void Chess::setStateString(const std::string &s)
{
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
        int index = y * 8 + x;
        char playerNumber = s[index] - '0';
        if (playerNumber) {
            square->setBit(PieceForPlayer(playerNumber - 1, Pawn));
        } else {
            square->setBit(nullptr);
        }
    });
}

void Chess::bitMovedFromTo(Bit &bit, BitHolder &src, BitHolder &dst)
{
    ChessSquare* dstSquare = static_cast<ChessSquare*>(&dst);
    endTurn();
}

ChessPiece Chess::getPieceType(const Bit& bit) const
{
    int tag = bit.gameTag();
    return static_cast<ChessPiece>(tag & 127);
}

bool Chess::isWhitePiece(const Bit& bit) const
{
    return bit.gameTag() < 128;
}

BitboardElement Chess::getOccupiedSquares() const
{
    uint64_t occupied = 0;
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
        if (square->bit()) {
            int index = y * 8 + x;
            occupied |= (1ULL << index);
        }
    });
    return BitboardElement(occupied);
}

BitboardElement Chess::getWhitePieces() const
{
    uint64_t white = 0;
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
        if (square->bit() && isWhitePiece(*square->bit())) {
            int index = y * 8 + x;
            white |= (1ULL << index);
        }
    });
    return BitboardElement(white);
}

BitboardElement Chess::getBlackPieces() const
{
    uint64_t black = 0;
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
        if (square->bit() && !isWhitePiece(*square->bit())) {
            int index = y * 8 + x;
            black |= (1ULL << index);
        }
    });
    return BitboardElement(black);
}

bool Chess::isSquareOccupied(int square) const
{
    int x = square % 8;
    int y = square / 8;
    ChessSquare* sq = _grid->getSquare(x, y);
    return sq && sq->bit() != nullptr;
}

bool Chess::isSquareOccupiedByColor(int square, bool white) const
{
    int x = square % 8;
    int y = square / 8;
    ChessSquare* sq = _grid->getSquare(x, y);
    if (!sq || !sq->bit()) return false;
    return isWhitePiece(*sq->bit()) == white;
}

void Chess::generatePawnMoves(int square, bool isWhite, std::vector<BitMove>& moves)
{
    int file = square % 8;
    int rank = square / 8;

    int direction = isWhite ? 1 : -1;
    int startRank = isWhite ? 1 : 6;

    int forwardSquare = square + direction * 8;
    if (forwardSquare >= 0 && forwardSquare < 64 && !isSquareOccupied(forwardSquare)) {
        moves.push_back(BitMove(square, forwardSquare, Pawn));

        if (rank == startRank) {
            int doubleForwardSquare = square + direction * 16;
            if (!isSquareOccupied(doubleForwardSquare)) {
                moves.push_back(BitMove(square, doubleForwardSquare, Pawn));
            }
        }
    }

    int captureLeft = square + direction * 8 - 1;
    int captureRight = square + direction * 8 + 1;

    if (file > 0 && captureLeft >= 0 && captureLeft < 64) {
        if (isSquareOccupied(captureLeft) && isSquareOccupiedByColor(captureLeft, !isWhite)) {
            moves.push_back(BitMove(square, captureLeft, Pawn));
        }
    }

    if (file < 7 && captureRight >= 0 && captureRight < 64) {
        if (isSquareOccupied(captureRight) && isSquareOccupiedByColor(captureRight, !isWhite)) {
            moves.push_back(BitMove(square, captureRight, Pawn));
        }
    }
}

void Chess::generateKnightMoves(int square, std::vector<BitMove>& moves)
{
    int file = square % 8;
    int rank = square / 8;

    const int knightOffsets[8][2] = {
        {-2, -1}, {-2, 1}, {-1, -2}, {-1, 2},
        {1, -2}, {1, 2}, {2, -1}, {2, 1}
    };

    for (int i = 0; i < 8; i++) {
        int newFile = file + knightOffsets[i][0];
        int newRank = rank + knightOffsets[i][1];

        if (newFile >= 0 && newFile < 8 && newRank >= 0 && newRank < 8) {
            int targetSquare = newRank * 8 + newFile;

            if (!isSquareOccupied(targetSquare) ||
                (isSquareOccupied(targetSquare))) {
                moves.push_back(BitMove(square, targetSquare, Knight));
            }
        }
    }
}

void Chess::generateKingMoves(int square, std::vector<BitMove>& moves)
{
    int file = square % 8;
    int rank = square / 8;

    const int kingOffsets[8][2] = {
        {-1, -1}, {-1, 0}, {-1, 1},
        {0, -1},           {0, 1},
        {1, -1},  {1, 0},  {1, 1}
    };

    for (int i = 0; i < 8; i++) {
        int newFile = file + kingOffsets[i][0];
        int newRank = rank + kingOffsets[i][1];

        if (newFile >= 0 && newFile < 8 && newRank >= 0 && newRank < 8) {
            int targetSquare = newRank * 8 + newFile;

            if (!isSquareOccupied(targetSquare) ||
                (isSquareOccupied(targetSquare))) {
                moves.push_back(BitMove(square, targetSquare, King));
            }
        }
    }
}
