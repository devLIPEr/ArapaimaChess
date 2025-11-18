#include "types.h"

Bitboard shift(Bitboard bb, int amount){
    if(amount > 0) return (bb << amount);
    return (bb >> -amount);
}

CastlingRights operator|(CastlingRights a, CastlingRights b){
    return static_cast<CastlingRights>(static_cast<int>(a) | static_cast<int>(b));
}
CastlingRights operator&(CastlingRights a, CastlingRights b){
    return static_cast<CastlingRights>(static_cast<int>(a) & static_cast<int>(b));
}
CastlingRights& operator|=(CastlingRights& a, CastlingRights b){
    return a= a |b;
}

/// @brief Create a move object
/// @param from index of the square piece comes from
/// @param to index of the square piece goes to
/// @param piece piece that is moving
/// @param capture_piece piece that is being captured
/// @param promotion_piece piece to promote to
/// @param passant en passant square if played
/// @param castling castling direction if played
/// @return new move
Move create_move(u8 from, u8 to, u8 piece, u8 capture_piece, u8 promotion_piece, u8 passant, u8 castling){
    Move move;
    move.from = from;
    move.to = to;
    move.piece = piece;
    move.capture_piece = capture_piece;
    move.promotion_piece = promotion_piece;
    move.set_en_passant(passant);
    move.set_castling(castling);
    return move;
}