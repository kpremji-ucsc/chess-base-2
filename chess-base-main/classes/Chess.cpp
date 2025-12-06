#include "Chess.h"
#include <limits>
#include <cmath>
#include <cstring>
#include <iostream>

#ifdef _MSC_VER
#include <intrin.h>
#pragma intrinsic(_BitScanForward64)

inline int ctz64(uint64_t value) {
    unsigned long index;
    _BitScanForward64(&index, value);
    return (int)index;
}
#else
inline int ctz64(uint64_t value) {
    return __builtin_ctzll(value);
}
#endif

Chess::Chess() : _countMoves(0)
{
    _grid = new Grid(8, 8);

    static bool magicInitialized = false;
    if (!magicInitialized) {
        MagicBitboards::initialize();
        magicInitialized = true;
    }

    _bitboardLookup['P'] = 0;
    _bitboardLookup['N'] = 1;
    _bitboardLookup['B'] = 2;
    _bitboardLookup['R'] = 3;
    _bitboardLookup['Q'] = 4;
    _bitboardLookup['K'] = 5;
    _bitboardLookup['p'] = 6;
    _bitboardLookup['n'] = 7;
    _bitboardLookup['b'] = 8;
    _bitboardLookup['r'] = 9;
    _bitboardLookup['q'] = 10;
    _bitboardLookup['k'] = 11;
    _bitboardLookup['0'] = 12;

    std::memset(_bitboards, 0, sizeof(_bitboards));
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

    if (gameHasAI()) {
        setAIPlayer(AI_PLAYER);
    }

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
        case Bishop:
            generateBishopMoves(srcSquareIndex, moves);
            break;
        case Rook:
            generateRookMoves(srcSquareIndex, moves);
            break;
        case Queen:
            generateQueenMoves(srcSquareIndex, moves);
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

void Chess::generateBishopMoves(int square, std::vector<BitMove>& moves)
{
    int file = square % 8;
    int rank = square / 8;

    const int directions[4][2] = {
        {1, 1}, {-1, 1}, {1, -1}, {-1, -1}
    };

    for (int dir = 0; dir < 4; dir++) {
        int newFile = file + directions[dir][0];
        int newRank = rank + directions[dir][1];

        while (newFile >= 0 && newFile < 8 && newRank >= 0 && newRank < 8) {
            int targetSquare = newRank * 8 + newFile;

            if (!isSquareOccupied(targetSquare)) {
                moves.push_back(BitMove(square, targetSquare, Bishop));
            } else {
                moves.push_back(BitMove(square, targetSquare, Bishop));
                break;
            }

            newFile += directions[dir][0];
            newRank += directions[dir][1];
        }
    }
}

void Chess::generateRookMoves(int square, std::vector<BitMove>& moves)
{
    int file = square % 8;
    int rank = square / 8;

    const int directions[4][2] = {
        {0, 1}, {0, -1}, {1, 0}, {-1, 0}
    };

    for (int dir = 0; dir < 4; dir++) {
        int newFile = file + directions[dir][0];
        int newRank = rank + directions[dir][1];

        while (newFile >= 0 && newFile < 8 && newRank >= 0 && newRank < 8) {
            int targetSquare = newRank * 8 + newFile;

            if (!isSquareOccupied(targetSquare)) {
                moves.push_back(BitMove(square, targetSquare, Rook));
            } else {
                moves.push_back(BitMove(square, targetSquare, Rook));
                break;
            }

            newFile += directions[dir][0];
            newRank += directions[dir][1];
        }
    }
}

void Chess::generateQueenMoves(int square, std::vector<BitMove>& moves)
{
    generateBishopMoves(square, moves);
    generateRookMoves(square, moves);
}

void Chess::generateMovesForPiece(int square, std::vector<BitMove>& moves)
{
    ChessSquare* sq = _grid->getSquare(square % 8, square / 8);
    if (!sq || !sq->bit()) return;

    Bit* piece = sq->bit();
    ChessPiece pieceType = getPieceType(*piece);
    bool isWhite = isWhitePiece(*piece);

    switch (pieceType) {
        case Pawn:
            generatePawnMoves(square, isWhite, moves);
            break;
        case Knight:
            generateKnightMoves(square, moves);
            break;
        case Bishop:
            generateBishopMoves(square, moves);
            break;
        case Rook:
            generateRookMoves(square, moves);
            break;
        case Queen:
            generateQueenMoves(square, moves);
            break;
        case King:
            generateKingMoves(square, moves);
            break;
        default:
            break;
    }
}

std::vector<BitMove> Chess::getAllLegalMoves(bool forWhite)
{
    std::vector<BitMove> allMoves;

    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
        if (square->bit()) {
            Bit* piece = square->bit();
            bool pieceIsWhite = isWhitePiece(*piece);

            if (pieceIsWhite == forWhite) {
                int squareIndex = y * 8 + x;
                std::vector<BitMove> pieceMoves;
                generateMovesForPiece(squareIndex, pieceMoves);

                for (const BitMove& move : pieceMoves) {
                    int toX = move.to % 8;
                    int toY = move.to / 8;
                    ChessSquare* destSquare = _grid->getSquare(toX, toY);

                    if (!destSquare->bit() ||
                        isWhitePiece(*destSquare->bit()) != forWhite) {
                        allMoves.push_back(move);
                    }
                }
            }
        }
    });

    return allMoves;
}

BitHolder& Chess::getHolderAt(int x, int y) {
    return *_grid->getSquare(x, y);
}

void Chess::buildBitboardsFromState(const std::string& state) {
    for (int i = 0; i < e_numBitboards; i++) {
        _bitboards[i] = 0;
    }

    for (int i = 0; i < 64; i++) {
        int bitIndex = _bitboardLookup[state[i]];
        _bitboards[bitIndex] |= 1ULL << i;
    }
}

std::vector<BitMove> Chess::generateAllMoves(const std::string& state, int playerColor) {
    std::vector<BitMove> moves;
    moves.reserve(32);

    buildBitboardsFromState(state);

    uint64_t occupied = 0;
    for (int i = 0; i < e_numBitboards - 1; i++) {
        occupied |= _bitboards[i];
    }

    bool isWhite = (playerColor == WHITE);
    int startIdx = isWhite ? 0 : 6;
    int endIdx = isWhite ? 6 : 12;

    uint64_t friendlyPieces = 0;
    uint64_t enemyPieces = 0;
    for (int i = 0; i < 6; i++) {
        if (isWhite) {
            friendlyPieces |= _bitboards[i];
            enemyPieces |= _bitboards[i + 6];
        } else {
            friendlyPieces |= _bitboards[i + 6];
            enemyPieces |= _bitboards[i];
        }
    }

    for (int pieceType = startIdx; pieceType < endIdx; pieceType++) {
        uint64_t pieces = _bitboards[pieceType];

        while (pieces) {
            int square = ctz64(pieces);
            pieces &= pieces - 1;

            uint64_t attacks = 0;
            ChessPiece piece = static_cast<ChessPiece>((pieceType % 6) + 1);

            switch (piece) {
                case Pawn: {
                    int direction = isWhite ? 8 : -8;
                    int startRank = isWhite ? 1 : 6;

                    int forward = square + direction;
                    if (forward >= 0 && forward < 64 && !(occupied & (1ULL << forward))) {
                        moves.push_back(BitMove(square, forward, Pawn));

                        if ((square / 8) == startRank) {
                            int doubleForward = square + 2 * direction;
                            if (!(occupied & (1ULL << doubleForward))) {
                                moves.push_back(BitMove(square, doubleForward, Pawn));
                            }
                        }
                    }

                    attacks = MagicBitboards::getPawnAttacks(square, isWhite);
                    attacks &= enemyPieces;
                    break;
                }
                case Knight:
                    attacks = MagicBitboards::getKnightAttacks(square);
                    attacks &= ~friendlyPieces;
                    break;
                case Bishop:
                    attacks = MagicBitboards::getBishopAttacks(square, occupied);
                    attacks &= ~friendlyPieces;
                    break;
                case Rook:
                    attacks = MagicBitboards::getRookAttacks(square, occupied);
                    attacks &= ~friendlyPieces;
                    break;
                case Queen:
                    attacks = MagicBitboards::getQueenAttacks(square, occupied);
                    attacks &= ~friendlyPieces;
                    break;
                case King:
                    attacks = MagicBitboards::getKingAttacks(square);
                    attacks &= ~friendlyPieces;
                    break;
                default:
                    break;
            }

            while (attacks) {
                int targetSquare = ctz64(attacks);
                attacks &= attacks - 1;
                moves.push_back(BitMove(square, targetSquare, piece));
            }
        }
    }

    return moves;
}

int Chess::evaluateBoard(const std::string& state) {
    static std::map<char, int> evaluateScores = {
        {'P', 100}, {'p', -100},
        {'N', 320}, {'n', -320},
        {'B', 330}, {'b', -330},
        {'R', 500}, {'r', -500},
        {'Q', 900}, {'q', -900},
        {'K', 20000}, {'k', -20000},
        {'0', 0}
    };

    int value = 0;
    for (char ch : state) {
        value += evaluateScores[ch];
    }

    return value;
}

int Chess::negamax(std::string& state, int depth, int alpha, int beta, int playerColor) {
    _countMoves++;

    if (depth == 0) {
        return evaluateBoard(state) * playerColor;
    }

    auto newMoves = generateAllMoves(state, playerColor);


    if (newMoves.empty()) {
        return negInfinite;
    }

    int bestVal = negInfinite;

    for (auto move : newMoves) {
        char boardSave = state[move.to];
        char pieceMoving = state[move.from];

        state[move.to] = pieceMoving;
        state[move.from] = '0';

        bestVal = std::max(bestVal, -negamax(state, depth - 1, -beta, -alpha, -playerColor));

        state[move.from] = pieceMoving;
        state[move.to] = boardSave;

        alpha = std::max(alpha, bestVal);
        if (alpha >= beta) {
            break;
        }
    }

    return bestVal;
}

void Chess::updateAI()
{
    if (!gameHasAI()) return;

    Player* aiPlayer = getCurrentPlayer();
    int playerColor = (aiPlayer->playerNumber() == 0) ? WHITE : BLACK;

    std::string state = stateString();
    _countMoves = 0;

    _moves = generateAllMoves(state, playerColor);

    if (_moves.empty()) {
        endTurn();
        return;
    }

    int bestVal = negInfinite;
    BitMove bestMove;

    for (auto move : _moves) {
        char boardSave = state[move.to];
        char pieceMoving = state[move.from];

        state[move.to] = pieceMoving;
        state[move.from] = '0';

        int moveVal = -negamax(state, 3, negInfinite, posInfinite, -playerColor);

        state[move.from] = pieceMoving;
        state[move.to] = boardSave;

        if (moveVal > bestVal) {
            bestMove = move;
            bestVal = moveVal;
        }
    }

    if (bestVal != negInfinite) {
        std::cout << "AI evaluated " << _countMoves << " positions" << std::endl;
        std::cout << "Best move score: " << bestVal << std::endl;

        int srcSquare = bestMove.from;
        int dstSquare = bestMove.to;
        BitHolder& src = getHolderAt(srcSquare & 7, srcSquare / 8);
        BitHolder& dst = getHolderAt(dstSquare & 7, dstSquare / 8);
        Bit* bit = src.bit();

        if (bit) {
            dst.dropBitAtPoint(bit, ImVec2(0, 0));
            src.setBit(nullptr);
            bitMovedFromTo(*bit, src, dst);
        }
    }
}
