#pragma once

#include <cstdint>
#include <array>

// Magic bitboards for fast sliding piece move generation
// This uses pre-computed magic numbers and lookup tables

namespace MagicBitboards {

// Constants
constexpr int ROOK_BITS[64] = {
    12, 11, 11, 11, 11, 11, 11, 12,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    12, 11, 11, 11, 11, 11, 11, 12
};

constexpr int BISHOP_BITS[64] = {
    6, 5, 5, 5, 5, 5, 5, 6,
    5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 7, 7, 7, 7, 5, 5,
    5, 5, 7, 9, 9, 7, 5, 5,
    5, 5, 7, 9, 9, 7, 5, 5,
    5, 5, 7, 7, 7, 7, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5,
    6, 5, 5, 5, 5, 5, 5, 6
};

// Attack tables
extern uint64_t ROOK_ATTACKS[64][4096];
extern uint64_t BISHOP_ATTACKS[64][512];
extern uint64_t ROOK_MASKS[64];
extern uint64_t BISHOP_MASKS[64];
extern uint64_t ROOK_MAGICS[64];
extern uint64_t BISHOP_MAGICS[64];

// Non-sliding piece attacks
extern uint64_t KNIGHT_ATTACKS[64];
extern uint64_t KING_ATTACKS[64];
extern uint64_t PAWN_ATTACKS[2][64]; // [color][square]

// Initialize all tables
void initialize();

// Get attack bitboards
inline uint64_t getRookAttacks(int square, uint64_t occupied) {
    occupied &= ROOK_MASKS[square];
    occupied *= ROOK_MAGICS[square];
    occupied >>= (64 - ROOK_BITS[square]);
    return ROOK_ATTACKS[square][occupied];
}

inline uint64_t getBishopAttacks(int square, uint64_t occupied) {
    occupied &= BISHOP_MASKS[square];
    occupied *= BISHOP_MAGICS[square];
    occupied >>= (64 - BISHOP_BITS[square]);
    return BISHOP_ATTACKS[square][occupied];
}

inline uint64_t getQueenAttacks(int square, uint64_t occupied) {
    return getRookAttacks(square, occupied) | getBishopAttacks(square, occupied);
}

inline uint64_t getKnightAttacks(int square) {
    return KNIGHT_ATTACKS[square];
}

inline uint64_t getKingAttacks(int square) {
    return KING_ATTACKS[square];
}

inline uint64_t getPawnAttacks(int square, bool isWhite) {
    return PAWN_ATTACKS[isWhite ? 0 : 1][square];
}

// Helper functions for mask generation
uint64_t generateRookMask(int square);
uint64_t generateBishopMask(int square);
uint64_t generateRookAttacks(int square, uint64_t occupied);
uint64_t generateBishopAttacks(int square, uint64_t occupied);

// Helper to set occupancy from index
uint64_t setOccupancy(int index, int bitsInMask, uint64_t mask);

} // namespace MagicBitboards

