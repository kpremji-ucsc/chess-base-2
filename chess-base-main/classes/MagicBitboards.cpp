#include "MagicBitboards.h"
#include <cstring>

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

namespace MagicBitboards {

// Attack tables (will be initialized)
uint64_t ROOK_ATTACKS[64][4096];
uint64_t BISHOP_ATTACKS[64][512];
uint64_t ROOK_MASKS[64];
uint64_t BISHOP_MASKS[64];
uint64_t KNIGHT_ATTACKS[64];
uint64_t KING_ATTACKS[64];
uint64_t PAWN_ATTACKS[2][64];

// Pre-computed magic numbers for rooks
uint64_t ROOK_MAGICS[64] = {
    0x0080001020400080ULL, 0x0040001000200040ULL, 0x0080081000200080ULL, 0x0080040800100080ULL,
    0x0080020400080080ULL, 0x0080010200040080ULL, 0x0080008001000200ULL, 0x0080002040800100ULL,
    0x0000800020400080ULL, 0x0000400020005000ULL, 0x0000801000200080ULL, 0x0000800800100080ULL,
    0x0000800400080080ULL, 0x0000800200040080ULL, 0x0000800100020080ULL, 0x0000800040800100ULL,
    0x0000208000400080ULL, 0x0000404000201000ULL, 0x0000808010000800ULL, 0x0000808008000400ULL,
    0x0000808004000200ULL, 0x0000808002000100ULL, 0x0000808001000100ULL, 0x0000408000800100ULL,
    0x0000204000808000ULL, 0x0000200040008080ULL, 0x0000100080008080ULL, 0x0000080080008080ULL,
    0x0000040080008080ULL, 0x0000020080008080ULL, 0x0000010080008080ULL, 0x0000008080008080ULL,
    0x0000802000808000ULL, 0x0000800800808000ULL, 0x0000800400808000ULL, 0x0000800200808000ULL,
    0x0000800100808000ULL, 0x0000800080808000ULL, 0x0000800040808000ULL, 0x0000800020808000ULL,
    0x0000800010808000ULL, 0x0000800008808000ULL, 0x0000800004808000ULL, 0x0000800002808000ULL,
    0x0000800001808000ULL, 0x0000800000808000ULL, 0x0000800000408000ULL, 0x0000800000208000ULL,
    0x0000800000108000ULL, 0x0000800000088000ULL, 0x0000800000048000ULL, 0x0000800000028000ULL,
    0x0000800000018000ULL, 0x0000800000008000ULL, 0x0000800000004000ULL, 0x0000800000002000ULL,
    0x0000408001000100ULL, 0x0000208001000100ULL, 0x0000108001000100ULL, 0x0000088001000100ULL,
    0x0000048001000100ULL, 0x0000028001000100ULL, 0x0000018001000100ULL, 0x0000008001000100ULL
};

// Pre-computed magic numbers for bishops
uint64_t BISHOP_MAGICS[64] = {
    0x0002020202020200ULL, 0x0002020202020000ULL, 0x0004010202000000ULL, 0x0004040080000000ULL,
    0x0001104000000000ULL, 0x0000821040000000ULL, 0x0000410410400000ULL, 0x0000104104104000ULL,
    0x0000040404040400ULL, 0x0000020202020200ULL, 0x0000040102020000ULL, 0x0000040400800000ULL,
    0x0000011040000000ULL, 0x0000008210400000ULL, 0x0000004104104000ULL, 0x0000002082082000ULL,
    0x0004000808080800ULL, 0x0002000404040400ULL, 0x0001000202020200ULL, 0x0000800802004000ULL,
    0x0000800400A00000ULL, 0x0000200100884000ULL, 0x0000400082082000ULL, 0x0000200041041000ULL,
    0x0002080010101000ULL, 0x0001040008080800ULL, 0x0000208004010400ULL, 0x0000404004010200ULL,
    0x0000840000802000ULL, 0x0000404002011000ULL, 0x0000808001041000ULL, 0x0000404000820800ULL,
    0x0001041000202000ULL, 0x0000820800101000ULL, 0x0000104400080800ULL, 0x0000020080080080ULL,
    0x0000404040040100ULL, 0x0000808100020100ULL, 0x0001010100020800ULL, 0x0000808080010400ULL,
    0x0000820820004000ULL, 0x0000410410002000ULL, 0x0000082088001000ULL, 0x0000002011000800ULL,
    0x0000080100400400ULL, 0x0001010101000200ULL, 0x0002020202000400ULL, 0x0001010101000200ULL,
    0x0000410410400000ULL, 0x0000208208200000ULL, 0x0000002084100000ULL, 0x0000000020880000ULL,
    0x0000001002020000ULL, 0x0000040408020000ULL, 0x0004040404040000ULL, 0x0002020202020000ULL,
    0x0000104104104000ULL, 0x0000002082082000ULL, 0x0000000020841000ULL, 0x0000000000208800ULL,
    0x0000000010020200ULL, 0x0000000404080200ULL, 0x0000040404040400ULL, 0x0002020202020200ULL
};

uint64_t generateRookMask(int square) {
    uint64_t mask = 0ULL;
    int rank = square / 8;
    int file = square % 8;
    
    // North
    for (int r = rank + 1; r < 7; r++) mask |= (1ULL << (r * 8 + file));
    // South
    for (int r = rank - 1; r > 0; r--) mask |= (1ULL << (r * 8 + file));
    // East
    for (int f = file + 1; f < 7; f++) mask |= (1ULL << (rank * 8 + f));
    // West
    for (int f = file - 1; f > 0; f--) mask |= (1ULL << (rank * 8 + f));
    
    return mask;
}

uint64_t generateBishopMask(int square) {
    uint64_t mask = 0ULL;
    int rank = square / 8;
    int file = square % 8;
    
    // NE
    for (int r = rank + 1, f = file + 1; r < 7 && f < 7; r++, f++)
        mask |= (1ULL << (r * 8 + f));
    // NW
    for (int r = rank + 1, f = file - 1; r < 7 && f > 0; r++, f--)
        mask |= (1ULL << (r * 8 + f));
    // SE
    for (int r = rank - 1, f = file + 1; r > 0 && f < 7; r--, f++)
        mask |= (1ULL << (r * 8 + f));
    // SW
    for (int r = rank - 1, f = file - 1; r > 0 && f > 0; r--, f--)
        mask |= (1ULL << (r * 8 + f));
    
    return mask;
}

uint64_t generateRookAttacks(int square, uint64_t occupied) {
    uint64_t attacks = 0ULL;
    int rank = square / 8;
    int file = square % 8;
    
    // North
    for (int r = rank + 1; r < 8; r++) {
        attacks |= (1ULL << (r * 8 + file));
        if (occupied & (1ULL << (r * 8 + file))) break;
    }
    // South
    for (int r = rank - 1; r >= 0; r--) {
        attacks |= (1ULL << (r * 8 + file));
        if (occupied & (1ULL << (r * 8 + file))) break;
    }
    // East
    for (int f = file + 1; f < 8; f++) {
        attacks |= (1ULL << (rank * 8 + f));
        if (occupied & (1ULL << (rank * 8 + f))) break;
    }
    // West
    for (int f = file - 1; f >= 0; f--) {
        attacks |= (1ULL << (rank * 8 + f));
        if (occupied & (1ULL << (rank * 8 + f))) break;
    }
    
    return attacks;
}

uint64_t generateBishopAttacks(int square, uint64_t occupied) {
    uint64_t attacks = 0ULL;
    int rank = square / 8;
    int file = square % 8;

    // NE
    for (int r = rank + 1, f = file + 1; r < 8 && f < 8; r++, f++) {
        attacks |= (1ULL << (r * 8 + f));
        if (occupied & (1ULL << (r * 8 + f))) break;
    }
    // NW
    for (int r = rank + 1, f = file - 1; r < 8 && f >= 0; r++, f--) {
        attacks |= (1ULL << (r * 8 + f));
        if (occupied & (1ULL << (r * 8 + f))) break;
    }
    // SE
    for (int r = rank - 1, f = file + 1; r >= 0 && f < 8; r--, f++) {
        attacks |= (1ULL << (r * 8 + f));
        if (occupied & (1ULL << (r * 8 + f))) break;
    }
    // SW
    for (int r = rank - 1, f = file - 1; r >= 0 && f >= 0; r--, f--) {
        attacks |= (1ULL << (r * 8 + f));
        if (occupied & (1ULL << (r * 8 + f))) break;
    }

    return attacks;
}

uint64_t setOccupancy(int index, int bitsInMask, uint64_t mask) {
    uint64_t occupancy = 0ULL;

    for (int count = 0; count < bitsInMask; count++) {
        int square = ctz64(mask);
        mask &= mask - 1;

        if (index & (1 << count)) {
            occupancy |= (1ULL << square);
        }
    }

    return occupancy;
}

void initializeKnightAttacks() {
    for (int square = 0; square < 64; square++) {
        uint64_t attacks = 0ULL;
        int rank = square / 8;
        int file = square % 8;

        int offsets[8][2] = {
            {-2, -1}, {-2, 1}, {-1, -2}, {-1, 2},
            {1, -2}, {1, 2}, {2, -1}, {2, 1}
        };

        for (int i = 0; i < 8; i++) {
            int newRank = rank + offsets[i][0];
            int newFile = file + offsets[i][1];

            if (newRank >= 0 && newRank < 8 && newFile >= 0 && newFile < 8) {
                attacks |= (1ULL << (newRank * 8 + newFile));
            }
        }

        KNIGHT_ATTACKS[square] = attacks;
    }
}

void initializeKingAttacks() {
    for (int square = 0; square < 64; square++) {
        uint64_t attacks = 0ULL;
        int rank = square / 8;
        int file = square % 8;

        int offsets[8][2] = {
            {-1, -1}, {-1, 0}, {-1, 1},
            {0, -1},           {0, 1},
            {1, -1},  {1, 0},  {1, 1}
        };

        for (int i = 0; i < 8; i++) {
            int newRank = rank + offsets[i][0];
            int newFile = file + offsets[i][1];

            if (newRank >= 0 && newRank < 8 && newFile >= 0 && newFile < 8) {
                attacks |= (1ULL << (newRank * 8 + newFile));
            }
        }

        KING_ATTACKS[square] = attacks;
    }
}

void initializePawnAttacks() {
    for (int square = 0; square < 64; square++) {
        int rank = square / 8;
        int file = square % 8;

        // White pawns (attack upward)
        uint64_t whiteAttacks = 0ULL;
        if (rank < 7) {
            if (file > 0) whiteAttacks |= (1ULL << ((rank + 1) * 8 + (file - 1)));
            if (file < 7) whiteAttacks |= (1ULL << ((rank + 1) * 8 + (file + 1)));
        }
        PAWN_ATTACKS[0][square] = whiteAttacks;

        // Black pawns (attack downward)
        uint64_t blackAttacks = 0ULL;
        if (rank > 0) {
            if (file > 0) blackAttacks |= (1ULL << ((rank - 1) * 8 + (file - 1)));
            if (file < 7) blackAttacks |= (1ULL << ((rank - 1) * 8 + (file + 1)));
        }
        PAWN_ATTACKS[1][square] = blackAttacks;
    }
}

void initialize() {
    // Initialize masks
    for (int square = 0; square < 64; square++) {
        ROOK_MASKS[square] = generateRookMask(square);
        BISHOP_MASKS[square] = generateBishopMask(square);
    }

    // Initialize rook attacks
    for (int square = 0; square < 64; square++) {
        int bits = ROOK_BITS[square];
        int permutations = 1 << bits;

        for (int index = 0; index < permutations; index++) {
            uint64_t occupancy = setOccupancy(index, bits, ROOK_MASKS[square]);
            uint64_t magicIndex = (occupancy * ROOK_MAGICS[square]) >> (64 - bits);
            ROOK_ATTACKS[square][magicIndex] = generateRookAttacks(square, occupancy);
        }
    }

    // Initialize bishop attacks
    for (int square = 0; square < 64; square++) {
        int bits = BISHOP_BITS[square];
        int permutations = 1 << bits;

        for (int index = 0; index < permutations; index++) {
            uint64_t occupancy = setOccupancy(index, bits, BISHOP_MASKS[square]);
            uint64_t magicIndex = (occupancy * BISHOP_MAGICS[square]) >> (64 - bits);
            BISHOP_ATTACKS[square][magicIndex] = generateBishopAttacks(square, occupancy);
        }
    }

    // Initialize non-sliding pieces
    initializeKnightAttacks();
    initializeKingAttacks();
    initializePawnAttacks();
}

}

