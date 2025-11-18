#include "move_generator.h"
#include "utils.h"
#include "evaluate.h"
#include "board.h"
#include "./magics/pext.h"
#include "./magics/fixed.h"

#include <cassert>
#include <algorithm>

using namespace std;

namespace arapaimachess{

/// @brief Create a MoveGenerator object
/// @tparam Magic the type of magic the move generator is using, see config.h
/// @param zobrist_table reference to Zobrist object
/// @param magic reference to Magic object
/// @param threads number of threads to use when generating perft results
template <typename Magic>
MoveGenerator<Magic>::MoveGenerator(Zobrist *zobrist_table, Magic *magic, int threads){
    assert(threads > 0 && zobrist_table != NULL && magic != NULL);
    this->zobrist_table = zobrist_table;
    this->magic = magic;
    this->num_threads = threads;
    memset(this->history, 0, 2*64*64*sizeof(u8));
}
template <typename Magic>
MoveGenerator<Magic>::MoveGenerator(){
    memset(this->history, 0, 2*64*64*sizeof(u8));
}

/// @brief Reset move history heuristic values
/// @tparam Magic the type of magic the move generator is using, see config.h
template <typename Magic>
void MoveGenerator<Magic>::reset_history(){
    memset(this->history, 0, 2*64*64*sizeof(u8));
}

/// @brief Add move to the history heuristic
/// @tparam Magic the type of magic the move generator is using, see config.h
/// @param player player playing the move
/// @param move move to add
/// @param depth depth where move was played
template <typename Magic>
void MoveGenerator<Magic>::add_history(Color player, Move move, int depth){
    this->history[player][move.from][move.to] += depth*depth;
    if (this->history[player][move.from][move.to] > MAX_HISTORY) {
        this->history[player][move.from][move.to] = MAX_HISTORY;
    }
}

template <typename Magic>
Bitboard MoveGenerator<Magic>::get_attack_rook(int sq, Bitboard occ){
    return this->magic->get_attack_rook(sq, occ);
}
template <typename Magic>
Bitboard MoveGenerator<Magic>::get_attack_bishop(int sq, Bitboard occ){
    return this->magic->get_attack_bishop(sq, occ);
}
template <typename Magic>
Bitboard MoveGenerator<Magic>::get_attack_knight(int sq){
    return this->magic->get_attack_knight(sq);
}
template <typename Magic>
Bitboard MoveGenerator<Magic>::get_attack_king(int sq){
    return this->magic->get_attack_king(sq);
}

/// @brief Check if there is a sliding attack
/// @tparam Magic the type of magic the move generator is using, see config.h
/// @param board bitboard of the square to check if attacked
/// @param opp_pieces bitboard of the opponent pieces
/// @param empty_pieces bitboard of empty squares
/// @param pc type of piece (BISHOP, ROOK)
/// @return true if the square is attacked by a slider, false otherwise
template <typename Magic>
bool MoveGenerator<Magic>::sliding_attack(Bitboard board, Bitboard opp_pieces, Bitboard empty_pieces, PieceType pc){
    Bitboard bb = 0;
    int index = __builtin_ctzll(board);
    if(pc == BISHOP){
        bb = this->get_attack_bishop(index, ~empty_pieces);
    }else if(pc == ROOK){
        bb = this->get_attack_rook(index, ~empty_pieces);
    }
    return ((bb & opp_pieces) > 0);
}

/// @brief Check if a square is attacked
/// @tparam Magic the type of magic the move generator is using, see config.h
/// @param square square index
/// @param board bitboard array
/// @param empty_pieces bitboard of the empty squares
/// @param color player's color
/// @return true if square is attacked, false otherwise
template <typename Magic>
bool MoveGenerator<Magic>::is_square_attacked(u8 square, Bitboard board[], Bitboard empty_pieces, Color color){
    int opp_pawn = (color) ? 0 : 6;
    Bitboard square_bb = (1ULL << square);
    
    if(sliding_attack(square_bb, board[opp_pawn+BISHOP-1] | board[opp_pawn+QUEEN-1], empty_pieces, BISHOP)) return true;

    if(sliding_attack(square_bb, board[opp_pawn+ROOK-1] | board[opp_pawn+QUEEN-1], empty_pieces, ROOK)) return true;

    if(board[opp_pawn+KNIGHT-1] & get_attack_knight(square)) return true;

    if(board[opp_pawn+KING-1] & get_attack_king(square)) return true;

    Bitboard pawn_attack = shift((board[opp_pawn+PAWN-1] & ~0x0101010101010101), ((color^1) ? -9 : 7)) | shift((board[opp_pawn+PAWN-1] & ~0x8080808080808080), ((color^1) ? -7 : 9));
    if(square_bb & pawn_attack) return true;

    return false;
}

/// @brief Extract capture moves of a given bitboard
/// @tparam Magic the type of magic the move generator is using, see config.h
/// @param orig move list to append the move to
/// @param boards bitboard array of all pieces
/// @param board bitboard of the captures
/// @param offset offset of the pawn move, used to calculate where the pawn came from
/// @param opp_pawn index of opponent pawn bitboard in the array
/// @param piece index of player's pawn in the bitboard array
/// @param promotion_piece piece to promote for
/// @param passant en passant square
template <typename Magic>
void MoveGenerator<Magic>::extract_pawn_captures(vector<Move> &orig, Bitboard boards[], Bitboard board, int offset, u8 opp_pawn, u8 piece, u8 promotion_piece, u8 passant){
    u8 capture_piece = opp_pawn+5, opp_king = opp_pawn+5;
    int index;
    while(board){
        capture_piece = opp_pawn;
        index = __builtin_ctzll(board);
        board &= board-1;
        if(passant == 0){
            if((1ULL << index) & boards[opp_pawn]){
                capture_piece = opp_pawn;
            }else if((1ULL << index) & boards[opp_pawn+1]){
                capture_piece = opp_pawn+1;
            }else if((1ULL << index) & boards[opp_pawn+2]){
                capture_piece = opp_pawn+2;
            }else if((1ULL << index) & boards[opp_pawn+3]){
                capture_piece = opp_pawn+3;
            }else if((1ULL << index) & boards[opp_pawn+4]){
                capture_piece = opp_pawn+4;
            }
            capture_piece = (capture_piece != opp_king ? capture_piece : 255);
        }
        orig.push_back(create_move(index+offset, index, piece, capture_piece, promotion_piece, passant, 0));
    }
}

/// @brief Extract pawn moves from a given bitboard
/// @tparam Magic the type of magic the move generator is using, see config.h
/// @param orig move list to append the move to
/// @param board bitboard of the captures
/// @param offset offset of the pawn move, used to calculate where the pawn came from
/// @param piece index of player's pawn in the bitboard array
/// @param promotion_piece piece to promote for
template <typename Magic>
void MoveGenerator<Magic>::extract_pawn_moves(vector<Move> &orig, Bitboard board, int offset, u8 piece, u8 promotion_piece){
    int index;
    while(board){
        index = __builtin_ctzll(board);
        board &= board-1;
        orig.push_back(create_move(index+offset, index, piece, 255, promotion_piece, 0, 0));
    }
}

/// @brief Generate all pawn moves for a given player in a given board
/// @tparam Magic the type of magic the move generator is using, see config.h
/// @param moves move list to append the moves to
/// @param board bitboard array of all pieces
/// @param color player's color
/// @param index index of player's pawn in the bitboard array
/// @param opp_pawn index of opponent pawn bitboard in the array
/// @param opp_pieces bitboard of the opponent pieces
/// @param empty_pieces bitboard of empty squares
/// @param en_passant_bb bitboard of en passant square
template <typename Magic>
void MoveGenerator<Magic>::generate_pawn_moves(vector<Move> &moves, Bitboard board[], Color color, int index, int opp_pawn, Bitboard opp_pieces, Bitboard empty_pieces, Bitboard en_passant_bb){
    Bitboard single_push = shift(board[index], (color ? -8 : 8)) & empty_pieces;
    Bitboard promotion = single_push & (color == WHITE ? RANK_8 : RANK_1);
    single_push = single_push & ~(color == WHITE ? RANK_8 : RANK_1);

    Bitboard double_push = shift(single_push & (color ? RANK_3 : RANK_6), (color ? -8 : 8)) & empty_pieces & (color == WHITE ? RANK_4 : RANK_5) & ~(color == WHITE ? RANK_3 : RANK_6);
    
    Bitboard left_attack = shift((board[index] & ~0x0101010101010101), (color ? -9 : 7));
    Bitboard left_capture = left_attack & opp_pieces;
    Bitboard left_en_passant = left_attack & en_passant_bb;
    
    Bitboard right_attack = shift((board[index] & ~0x8080808080808080), (color ? -7 : 9));
    Bitboard right_capture = right_attack & opp_pieces;
    Bitboard right_en_passant = right_attack & en_passant_bb;
    moves.reserve(
        moves.size()+
        __builtin_popcount(single_push)+
        __builtin_popcount(promotion)*4+
        __builtin_popcount(double_push)+
        __builtin_popcount(left_capture)+
        __builtin_popcount(left_en_passant)+
        __builtin_popcount(left_capture & (color == WHITE ? RANK_8 : RANK_1))*4+
        __builtin_popcount(right_capture)+
        __builtin_popcount(right_en_passant)+
        __builtin_popcount(right_capture & (color == WHITE ? RANK_8 : RANK_1))*4
    );
    extract_pawn_moves(moves, single_push, (color ? 8 : -8), index, 0);
    
    if(promotion){
        for(int type = KNIGHT; type < KING; type++){
            extract_pawn_moves(moves, promotion, (color ? 8 : -8), index, type-1);
        }
    }
    extract_pawn_moves(moves, double_push, (color ? 16 : -16), index, 0);
    extract_pawn_captures(moves, board, left_capture & ~(color == WHITE ? RANK_8 : RANK_1), (color ? 9 : -7), opp_pawn, index, 0, 0);
    extract_pawn_captures(moves, board, left_en_passant, (color ? 9 : -7), opp_pawn, index, 0, __builtin_ctzll(left_en_passant));
    if(left_capture & (color == WHITE ? RANK_8 : RANK_1)){
        for(int type = KNIGHT; type < KING; type++){
            extract_pawn_captures(moves, board, left_capture & (color == WHITE ? RANK_8 : RANK_1), (color ? 9 : -7), opp_pawn, index, type-1, 0);
        }
    }
    extract_pawn_captures(moves, board, right_capture & ~(color == WHITE ? RANK_8 : RANK_1), (color ? 7 : -9), opp_pawn, index, 0, 0);
    extract_pawn_captures(moves, board, right_en_passant, (color ? 7 : -9), opp_pawn, index, 0, __builtin_ctzll(right_en_passant));
    if(right_capture & (color == WHITE ? RANK_8 : RANK_1)){
        for(int type = KNIGHT; type < KING; type++){
            extract_pawn_captures(moves, board, right_capture & (color == WHITE ? RANK_8 : RANK_1), (color ? 7 : -9), opp_pawn, index, type-1, 0);
        }
    }
}

/// @brief Extract capture moves from a given bitboard
/// @tparam Magic the type of magic the move generator is using, see config.h
/// @param orig move list to append the move to
/// @param boards bitboard array of all pieces
/// @param board bitboard of the captures
/// @param from square the piece came from
/// @param opp_pawn index of opponent pawn bitboard in the array
/// @param piece index of player's pawn in the bitboard array
template <typename Magic>
void MoveGenerator<Magic>::extract_capture_moves(vector<Move> &orig, Bitboard boards[], Bitboard board, u8 from, u8 opp_pawn, u8 piece){
    u8 capture_piece = opp_pawn+5, opp_king = opp_pawn+5;
    int index;
    while(board){
        capture_piece = opp_pawn;
        index = __builtin_ctzll(board);

        if((1ULL << index) & boards[opp_pawn]){
            capture_piece = opp_pawn;
        }else if((1ULL << index) & boards[opp_pawn+1]){
            capture_piece = opp_pawn+1;
        }else if((1ULL << index) & boards[opp_pawn+2]){
            capture_piece = opp_pawn+2;
        }else if((1ULL << index) & boards[opp_pawn+3]){
            capture_piece = opp_pawn+3;
        }else if((1ULL << index) & boards[opp_pawn+4]){
            capture_piece = opp_pawn+4;
        }

        board &= board-1;
        orig.push_back(create_move(from, index, piece, (capture_piece != opp_king ? capture_piece : 255), 255, 0, 0));
    }
}

/// @brief Extract moves from a given bitboard
/// @tparam Magic the type of magic the move generator is using, see config.h
/// @param orig move list to append the move to
/// @param board bitboard of the moves
/// @param from square the piece came from
/// @param piece index of player's pawn in the bitboard array
template <typename Magic>
void MoveGenerator<Magic>::extract_moves(vector<Move> &orig, Bitboard board, u8 from, u8 piece){
    int index;
    while(board){
        index = __builtin_ctzll(board);
        board &= board-1;
        orig.push_back(create_move(from, index, piece, 255, 255, 0, 0));
    }
}

/// @brief Generate all knights moves for a given position
/// @tparam Magic the type of magic the move generator is using, see config.h
/// @param moves move list to append the moves to
/// @param board bitboard array of all pieces
/// @param piece_index index of player's knight in the bitboard array
/// @param opp_pawn index of opponent pawn bitboard in the array
/// @param opp_pieces bitboard of the opponent pieces
/// @param empty_pieces bitboard of empty squares
template <typename Magic>
void MoveGenerator<Magic>::generate_knight_moves(vector<Move> &moves, Bitboard board[], int piece_index, int opp_pawn, Bitboard opp_pieces, Bitboard empty_pieces){
    Bitboard knight = board[piece_index], knights_attacks, knight_move, knight_captures;
    int index;
    while(knight){
        index = __builtin_ctzll(knight);
        knights_attacks = get_attack_knight(index);

        knight_move = knights_attacks & empty_pieces;
        knight_captures = knights_attacks & opp_pieces;
        moves.reserve(moves.size() + __builtin_popcount(knight_move) + __builtin_popcount(knight_captures));
        extract_moves(moves, knight_move, index, piece_index);
        extract_capture_moves(moves, board, knight_captures, index, opp_pawn, piece_index);
        knight &= knight-1;
    }
}

/// @brief Check if a given player is in check
/// @tparam Magic the type of magic the move generator is using, see config.h
/// @param board bitboard array of all pieces
/// @param empty_pieces bitboard of empty squares
/// @param color player to check
/// @return true if player is in check, false otherwise
template <typename Magic>
bool MoveGenerator<Magic>::in_check(Bitboard board[], Bitboard empty_pieces, Color color){
    return is_square_attacked(__builtin_ctzll(board[(color ? WHITE_KING-1 : BLACK_KING-1)]), board, empty_pieces, color);
}

/// @brief Generate all king moves for the given player
/// @tparam Magic the type of magic the move generator is using, see config.h
/// @param moves move list to append the moves to
/// @param board bitboard array of all pieces
/// @param color player to generate moves
/// @param piece_index index of player's king in the bitboard array
/// @param opp_pawn index of opponent pawn bitboard in the array
/// @param all_pieces bitboard of all pieces
/// @param opp_pieces bitboard of the opponent pieces
/// @param empty_pieces bitboard of empty squares
/// @param crs castling rights
template <typename Magic>
void MoveGenerator<Magic>::generate_king_moves(vector<Move> &moves, Bitboard board[], Color color, int piece_index, int opp_pawn, Bitboard all_pieces, Bitboard opp_pieces, Bitboard empty_pieces, CastlingRights crs){
    int king_s = __builtin_ctzll(board[piece_index]);
    Bitboard king_moves = get_attack_king(king_s), king_square = board[piece_index];

    Bitboard king_captures = king_moves & opp_pieces;
    king_moves = king_moves & empty_pieces;
    moves.reserve(moves.size() + __builtin_popcount(king_moves) + __builtin_popcount(king_captures));
    extract_moves(moves, king_moves, king_s, piece_index);
    extract_capture_moves(moves, board, king_captures, king_s, opp_pawn, piece_index);

    CastlingRights oo = (color ? WHITE_OO : BLACK_OO), ooo = (color ? WHITE_OOO : BLACK_OOO);
    i8 castling_shift_oo = 1;
    i8 castling_shift_ooo = -1;
    
    if((crs & oo) && ((color ? 6917529027641081856ULL : 96ULL) & all_pieces) == 0 && !in_check(board, empty_pieces, color) && !is_square_attacked(__builtin_ctzll(shift(king_square, castling_shift_oo)), board, empty_pieces, color) && !is_square_attacked(__builtin_ctzll(shift(king_square, castling_shift_oo*2)), board, empty_pieces, color)){
        moves.push_back(create_move(king_s, __builtin_ctzll(shift(king_square, (castling_shift_oo*2))), piece_index, 255, 255, 0, 1));
    }
    if((crs & ooo) && ((color ? 1008806316530991104ULL : 14ULL) & all_pieces) == 0 && !in_check(board, empty_pieces, color) && !is_square_attacked(__builtin_ctzll(shift(king_square, castling_shift_ooo)), board, empty_pieces, color) && !is_square_attacked(__builtin_ctzll(shift(king_square, castling_shift_ooo*2)), board, empty_pieces, color)){
        moves.push_back(create_move(king_s, __builtin_ctzll(shift(king_square, (castling_shift_ooo*2))), piece_index, 255, 255, 0, 2));
    }
}

/// @brief Generate all sliding moves
/// @tparam Magic the type of magic the move generator is using, see config.h
/// @param moves move list to append the moves to
/// @param board bitboard array of all pieces
/// @param color player to generate moves
/// @param piece_index index of player's slider in the bitboard array
/// @param opp_pawn index of opponent pawn bitboard in the array
/// @param opp_pieces bitboard of the opponent pieces
/// @param empty_pieces bitboard of empty squares
template <typename Magic>
void MoveGenerator<Magic>::generate_sliding_moves(vector<Move> &moves, Bitboard board[], Color color, int piece_index, int opp_pawn, Bitboard opp_pieces, Bitboard empty_pieces){
    Bitboard sliding_piece = board[piece_index], pattern;
    int index, side = (color*6)-1;
    while(sliding_piece){
        index = __builtin_ctzll(sliding_piece);
        if(piece_index == BISHOP+side){
            pattern = get_attack_bishop(index, ~empty_pieces);
        }else if(piece_index == ROOK+side){
            pattern = get_attack_rook(index, ~empty_pieces);
        }else{
            pattern = get_attack_rook(index, ~empty_pieces) | get_attack_bishop(index, ~empty_pieces);
        }
        moves.reserve(moves.size() + __builtin_popcount(pattern & empty_pieces) + __builtin_popcount(pattern & opp_pieces));
        extract_moves(moves, pattern & empty_pieces, index, piece_index);
        extract_capture_moves(moves, board, pattern & opp_pieces, index, opp_pawn, piece_index);
        sliding_piece &= sliding_piece-1;
    }
}

/// @brief Generate pseudolegal moves
/// @tparam Magic the type of magic the move generator is using, see config.h
/// @param board bitboard array of all pieces
/// @param color player to generate moves
/// @param crs castling rights
/// @param eps en passant square
/// @return list of pseudolegal moves
template <typename Magic>
vector<Move> MoveGenerator<Magic>::pseudolegal_moves(Bitboard board[], Color color, CastlingRights crs, u8 eps){
    vector<Move> moves;
    int opp_pawn = NO_PIECE+((color^1)*6);
    int pawn = NO_PIECE+(color*6);
    int knight = pawn+1;
    int bishop = knight+1;
    int rook = bishop+1;
    int queen = rook+1;
    int king = queen+1;

    Bitboard all_pieces = 0, empty_pieces, opp_pieces = 0, en_passant_bb = (eps != 255) ? (1ULL << eps) : 0;
    for(int i = NO_PIECE; i < WHITE_KING; i++){
        all_pieces |= board[i];
    }
    for(int i = (color ? NO_PIECE : BLACK_KING); i < (color ? BLACK_KING : WHITE_KING); i++){
        opp_pieces |= board[i];
    }
    empty_pieces = ~all_pieces;

    generate_pawn_moves(moves, board, color, pawn, opp_pawn, opp_pieces, empty_pieces, en_passant_bb);
    
    generate_knight_moves(moves, board, knight, opp_pawn, opp_pieces, empty_pieces);
    
    generate_sliding_moves(moves, board, color, bishop, opp_pawn, opp_pieces, empty_pieces);
    generate_sliding_moves(moves, board, color, queen, opp_pawn, opp_pieces, empty_pieces);
    generate_sliding_moves(moves, board, color, rook, opp_pawn, opp_pieces, empty_pieces);
    
    generate_king_moves(moves, board, color, king, opp_pawn, all_pieces, opp_pieces, empty_pieces, crs);

    return moves;
}

/// @brief Generate legal moves
/// @tparam Magic the type of magic the move generator is using, see config.h
/// @param board bitboard array of all pieces
/// @param color player to generate moves
/// @param crs castling rights
/// @param eps en passant square
/// @return list of legal moves
template <typename Magic>
vector<Move> MoveGenerator<Magic>::legal_moves(Bitboard board[], Color color, CastlingRights crs, u8 eps){
    vector<Move> legal, moves = pseudolegal_moves(board, color, crs, eps);
    
    for(Move move: moves){
        CastlingRights cr = crs;
        u8 ep = eps;
        Bitboard board_copy[12];
        memcpy(board_copy, board, 12*sizeof(Bitboard));
        Board<Magic>::do_move(board_copy, move, color, cr, ep);

        Bitboard empty_pieces = 0;
        for(int i = NO_PIECE; i < WHITE_KING; i++){
            empty_pieces |= board_copy[i];
        }
        empty_pieces = ~empty_pieces;

        if(!in_check(board_copy, empty_pieces, color)){
            legal.push_back(move);
        }

        // Board<Magic>::undo_move(board_copy, move, color, cr, ep, crs, eps);
    }

    return legal;
}

/// @brief Filter move list to contain only captures
/// @tparam Magic the type of magic the move generator is using, see config.h
/// @param moves list of moves
/// @return list of captures moves
template <typename Magic>
vector<Move> MoveGenerator<Magic>::captures_moves(vector<Move> moves){
    vector<Move> captures;
    
    for(Move move: moves){
        if(move.capture_piece != 255){
            captures.push_back(move);
        }
    }

    return captures;
}

/// @brief Filter move list to contain only promotions
/// @tparam Magic the type of magic the move generator is using, see config.h
/// @param moves list of moves
/// @return list of promotions moves
template <typename Magic>
vector<Move> MoveGenerator<Magic>::promotions_moves(vector<Move> moves){
    vector<Move> promotions;
    
    for(Move move: moves){
        if(move.promotion_piece != 255 || move.promotion_piece != 0){
            promotions.push_back(move);
        }
    }

    return promotions;
}

/// @brief Order moves to contain {captures, promotions, non_captures}
/// @tparam Magic the type of magic the move generator is using, see config.h
/// @param board bitboard array of all pieces
/// @param moves list of moves
/// @param cr castling rights
/// @param ep en passant square
/// @param color player to order moves
/// @param captures_only get only capture, promotions and check moves
/// @return list of ordered moves
template <typename Magic>
vector<Move> MoveGenerator<Magic>::order_moves(Bitboard board[], vector<Move> moves, Color color, bool captures_only){
    vector<Move> captures = captures_moves(moves);
    vector<Move> promotions = promotions_moves(moves);
    vector<Move> ordered;
    ordered.reserve(captures.size());
    
    int capture_evals[captures.size()];
    int i = 0;
    for(Move m : captures){
        m.idx = i;
        capture_evals[m.idx] = 100*material_value[m.capture_piece] - material_value[m.piece];
        i++;
    }
    sort(captures.begin(), captures.end(), [this, &capture_evals](const Move &a, const Move &b){
        return capture_evals[a.idx] > capture_evals[b.idx];
    });

    for(Move move: captures){
        ordered.push_back(move);
    }

    if(promotions.size() > 0){
        int promotion_evals[promotions.size()];
        i = 0;
        for(Move m : promotions){
            m.idx = i;
            promotion_evals[m.idx] = material_value[m.promotion_piece];
            i++;
        }
        sort(promotions.begin(), promotions.end(), [this, &promotion_evals](const Move &a, const Move &b){
            return promotion_evals[a.idx] > promotion_evals[b.idx];
        });
    
        for(Move move: promotions){
            ordered.push_back(move);
        }
    }

    vector<Move> non_captures; non_captures.reserve(moves.size()-captures.size());
    for(Move move: moves){
        if(captures_only){
            if(move.capture_piece == 255 && (move.promotion_piece == 255 || move.promotion_piece == 0)){
                CastlingRights cr = NO_CASTLING;
                u8 ep = 255;
                Bitboard board_copy[12];
                memcpy(board_copy, board, 12*sizeof(Bitboard));
                Board<Magic>::do_move(board_copy, move, color, cr, ep);

                Bitboard empty_pieces = 0;
                for(int i = NO_PIECE; i < WHITE_KING; i++){
                    empty_pieces |= board_copy[i];
                }
                empty_pieces = ~empty_pieces;

                if(in_check(board_copy, empty_pieces, Color(color^1))){
                    non_captures.push_back(move);
                }
            }
        }else{
            non_captures.push_back(move);
        }
    }
    ordered.reserve(ordered.size()+non_captures.size());

    int evals[non_captures.size()];
    i = 0;
    for(Move m : non_captures){
        m.idx = i;
        evals[m.idx] = history[color][m.from][m.to];
        i++;
    }
    sort(non_captures.begin(), non_captures.end(), [this, &evals](const Move &a, const Move &b){
        return evals[a.idx] > evals[b.idx];
    });

    for(Move move: non_captures){
        ordered.push_back(move);
    }

    return ordered;
}

/// @brief Start perft test
/// @tparam Magic the type of magic the move generator is using, see config.h
/// @param depth depth to check
/// @param board bitboard array of all pieces
/// @param tt reference to transposition table object
/// @param color player to start the test from
/// @param crs castling rights
/// @param eps en passant square
/// @param piece_zob use fast update on the zobrist key
/// @param use_loop update zobrist key looping through all bitboards
/// @return amount of nodes in the perft test
template <typename Magic>
u64 MoveGenerator<Magic>::perft(int depth, Bitboard board[], Color color, TT &tt, CastlingRights &crs, uint8_t &eps, u64 *piece_zob, bool use_loop){
    if(depth == 0){
        return 1ULL;
    }
    
    u64 key = zob_key(*zobrist_table, board, color, crs, eps, piece_zob, use_loop);
    Entry curr_entry = tt.atomic_read(key);
    if(curr_entry.depth == depth && curr_entry.is_board_equal(key)){
        return curr_entry.count;
    }

    u64 nodes = 0;
    vector<Move> moves = pseudolegal_moves(board, color, crs, eps);
    Bitboard board_copy[12];
    for(Move move: moves){
        memcpy(board_copy, board, 12*sizeof(Bitboard));
        CastlingRights cr = crs;
        uint8_t ep = eps;

        Board<Magic>::do_move(board_copy, move, color, cr, ep);

        Bitboard empty_pieces = 0;
        for(int i = NO_PIECE; i < WHITE_KING; i++){
            empty_pieces |= board_copy[i];
        }
        empty_pieces = ~empty_pieces;

        u64 aux = 0;
        if(piece_zob != NULL && !use_loop){
            aux = (*piece_zob);
            if(in_range(move.get_en_passant(), 16, 47)){
                (*piece_zob) ^= (*zobrist_table)[move.capture_piece*64+(move.get_en_passant()+(color ? 8 : -8))];
            }else if(move.capture_piece != 255 && move.promotion_piece == 0){
                (*piece_zob) ^= (*zobrist_table)[move.capture_piece*64+move.to];
            }
            if(move.promotion_piece != 255){
                (*piece_zob) ^= (*zobrist_table)[move.promotion_piece*64+move.to];
            }
            (*piece_zob) ^= (*zobrist_table)[move.piece*64+move.from];
        }

        if(!in_check(board_copy, empty_pieces, color)){
            nodes += perft(depth-1, board_copy, Color(color^1), tt, cr, ep, piece_zob, false);
        }

        if(piece_zob != NULL && !use_loop){
            (*piece_zob) = aux;
        }
    }

    tt.add(key, Entry(depth, nodes, key));
    
    return nodes;
}

/// @brief Start parallelized perft test
/// @tparam Magic the type of magic the move generator is using, see config.h
/// @param depth depth to check
/// @param board bitboard array of all pieces
/// @param color player to start the test from
/// @param castling castling rights
/// @param en_passant en passant square
/// @param tt reference to transposition table object
/// @param use_piece_zob use fast update on the zobrist key
/// @return amount of nodes in the perft test
template <typename Magic>
u64 MoveGenerator<Magic>::perft_parallel(int depth, Bitboard board[], Color color, CastlingRights castling, u8 en_passant, TT &tt, bool use_piece_zob){
    if(depth == 0) return legal_moves(board, color, castling, en_passant).size();

    Bitboard boards_copy[num_threads][12];
    u64 pieces_zob[num_threads];
    memset(pieces_zob, 0, sizeof(u64)*num_threads);
    CastlingRights crs[num_threads];
    u8 eps[num_threads];
    for(int i = 0; i < num_threads; i++){
        memcpy(boards_copy[i], board, 12*sizeof(Bitboard));
        crs[i] = castling;
        eps[i] = en_passant;
    }
    
    u64 nodes = 0;
    vector<Move> moves = pseudolegal_moves(board, color, castling, en_passant);
    #pragma omp parallel for num_threads(num_threads) reduction(+:nodes)
    for(Move move: moves){
        int idx = omp_get_thread_num();

        Bitboard board_copy[12];
        memcpy(board_copy, boards_copy[idx], 12*sizeof(Bitboard));
        CastlingRights cr = crs[idx];
        u8 ep = eps[idx];

        Board<Magic>::do_move(board_copy, move, color, cr, ep);

        Bitboard empty_pieces = 0;
        for(int i = NO_PIECE; i < WHITE_KING; i++){
            empty_pieces |= board_copy[i];
        }
        empty_pieces = ~empty_pieces;

        if(!in_check(board_copy, empty_pieces, color)){
            if(use_piece_zob){
                nodes += perft(depth-1, board_copy, Color(color^1), tt, cr, ep, (pieces_zob+idx), use_piece_zob);
            }else{
                nodes += perft(depth-1, board_copy, Color(color^1), tt, cr, ep, NULL, false);
            }
        }
    }

    return nodes;
}

template class MoveGenerator<PEXT_Magic>;
template class MoveGenerator<FIXED_Magic>;

}
