#ifndef TYPES_H
#define TYPES_H

using i8 = signed char;
using u8 = unsigned char;
using u16 = unsigned short;
using u64 = unsigned long long;
using Bitboard = unsigned long long;

enum Color: u8{
    BLACK = 0,
    WHITE = 1,
    NO_COLOR
};

enum PieceType: u8{
    NO_TYPE = 0,
    PAWN = 1,
    KNIGHT,
    BISHOP,
    ROOK,
    QUEEN,
    KING
};

enum Pieces: u8{
    NO_PIECE = 0,
    
    BLACK_PAWN = 1,
    BLACK_KNIGHT,
    BLACK_BISHOP,
    BLACK_ROOK,
    BLACK_QUEEN,
    BLACK_KING,

    WHITE_PAWN,
    WHITE_KNIGHT,
    WHITE_BISHOP,
    WHITE_ROOK,
    WHITE_QUEEN,
    WHITE_KING
};

enum Piece_Value: int{
    PAWN_VALUE = 100,
    KNIGHT_VALUE = 300,
    BISHOP_VALUE = 300,
    ROOK_VALUE = 500,
    QUEEN_VALUE = 900,
    KING_VALUE = 1000,
};

enum Ranks: u64{
    RANK_8 = 255ULL,
    RANK_7 = 255ULL << 8,
    RANK_6 = 255ULL << 16,
    RANK_5 = 255ULL << 24,
    RANK_4 = 255ULL << 32,
    RANK_3 = 255ULL << 40,
    RANK_2 = 255ULL << 48,
    RANK_1 = 255ULL << 56
};

enum Files: u64{
    FILE_A = 9259542123273814144ULL >> 7,
    FILE_B = 9259542123273814144ULL >> 6,
    FILE_C = 9259542123273814144ULL >> 5,
    FILE_D = 9259542123273814144ULL >> 4,
    FILE_E = 9259542123273814144ULL >> 3,
    FILE_F = 9259542123273814144ULL >> 2,
    FILE_G = 9259542123273814144ULL >> 1,
    FILE_H = 9259542123273814144ULL
};

enum CastlingRights: u8{
    NO_CASTLING = 0,

    WHITE_OO  = 1,
    WHITE_OOO = WHITE_OO << 1,
    BLACK_OO  = WHITE_OO << 2,
    BLACK_OOO = WHITE_OO << 3,

    KING_SIDE      = WHITE_OO | BLACK_OO,
    QUEEN_SIDE     = WHITE_OOO | BLACK_OOO,
    WHITE_CASTLING = WHITE_OO | WHITE_OOO,
    BLACK_CASTLING = BLACK_OO | BLACK_OOO,
    ANY_CASTLING   = WHITE_CASTLING | BLACK_CASTLING
};

enum TT_FLAGS: u8{
    NO_FLAG = 0,
    TT_EXACT,
    TT_LOWER,
    TT_UPPER
};

// flags - CCEE_EEEE
struct Move{
    u8 from = 255, to = 255;
    u8 piece = 255, capture_piece = 255, promotion_piece = 255, flags = 255;
    u8 idx;
    
    constexpr u8 get_row_from() { return (this->from >> 3); }
    constexpr u8 get_row_to() { return (this->to >> 3); }
    constexpr u8 get_col_from() { return 7-(this->from & 7); }
    constexpr u8 get_col_to() { return 7-(this->to & 7); }
    constexpr u8 get_en_passant() { return this->flags & 63; }
    constexpr u8 get_castling() { return (this->flags & 192) >> 6; }

    constexpr void set_en_passant(u8 passant) { this->flags = (this->flags & ~63) | passant; }
    constexpr void set_castling(u8 castling) { this->flags = (this->flags & ~192) | (castling << 6); }

    bool operator==(const Move& other) const {
        return (
            this->from == other.from &&
            this->to == other.to &&
            this->piece == other.piece &&
            this->capture_piece == other.capture_piece &&
            this->promotion_piece == other.promotion_piece &&
            this->flags == other.flags
        );
    }
};

Bitboard shift(Bitboard bb, int amount);

CastlingRights operator|(CastlingRights a, CastlingRights b);
CastlingRights operator&(CastlingRights a, CastlingRights b);
CastlingRights& operator|=(CastlingRights& a, CastlingRights b);

Move create_move(u8 from, u8 to, u8 piece, u8 capture_piece, u8 promotion_piece, u8 passant, u8 castling);

struct PVLine{
    int cmove = 0;
    int eval[256] = {0};
    Move argmove[256];
    u8 flags[256] = {0};
};

const int eval_wdl[5] = {-2147400000, 0, 0, 0, 2147400000};

#endif