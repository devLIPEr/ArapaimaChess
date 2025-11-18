#include "board.h"
#include "utils.h"
#include "./magics/pext.h"
#include "./magics/fixed.h"
#include <cstring>
#include <cassert>

using namespace std;

namespace arapaimachess{

template <typename Magic>
Board<Magic>::Board(){}

/// @brief Create a Board object with the starting position
/// @tparam Magic the type of magic the move generator is using, see config.h
/// @param zobrist_table reference to the Zobrist object
/// @param move_generator reference to the MoveGenerator object
template <typename Magic>
Board<Magic>::Board(Zobrist *zobrist_table, MoveGenerator<Magic> *move_generator){
    assert(zobrist_table != NULL && move_generator != NULL);
    this->zobrist_table = zobrist_table;
    this->move_generator = move_generator;
    this->add_pieces();
    this->initialize_board("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0");
}

/// @brief Create a Board object with the given fen
/// @tparam Magic the type of magic the move generator is using, see config.h
/// @param zobrist_table reference to the Zobrist object
/// @param move_generator reference to the MoveGenerator object
/// @param board_fen fen to parse
template <typename Magic>
Board<Magic>::Board(Zobrist *zobrist_table, MoveGenerator<Magic> *move_generator, string board_fen){
    assert(zobrist_table != NULL && move_generator != NULL);
    this->zobrist_table = zobrist_table;
    this->move_generator = move_generator;
    this->add_pieces();
    this->initialize_board(board_fen);
}

/// @brief Count how many pieces there is in the current position
/// @tparam Magic the type of magic the move generator is using, see config.h
/// @return number of pieces (including both kings)
template <typename Magic>
unsigned int Board<Magic>::count_pieces(){
    unsigned int count = 0;
    for(int i = 0; i < 12; i++)
        count += __builtin_popcountll(board[i]);
    return count;
}

/// @brief Count how many pieces there is in a given position
/// @tparam Magic the type of magic the move generator is using, see config.h
/// @param board the bitboard array to count from
/// @return number of pieces (including both kings)
template <typename Magic>
unsigned int Board<Magic>::count_pieces(Bitboard board[]){
    unsigned int count = 0;
    for(int i = 0; i < 12; i++)
        count += __builtin_popcountll(board[i]);
    return count;
}

template <typename Magic>
constexpr void Board<Magic>::add_pieces(){
    piece[NO_PIECE] = '_';

    piece_type['p'] = PAWN;   piece['p'] = BLACK_PAWN;   piece[BLACK_PAWN]   = 'p';
    piece_type['n'] = KNIGHT; piece['n'] = BLACK_KNIGHT; piece[BLACK_KNIGHT] = 'n';
    piece_type['b'] = BISHOP; piece['b'] = BLACK_BISHOP; piece[BLACK_BISHOP] = 'b';
    piece_type['r'] = ROOK;   piece['r'] = BLACK_ROOK;   piece[BLACK_ROOK]   = 'r';
    piece_type['q'] = QUEEN;  piece['q'] = BLACK_QUEEN;  piece[BLACK_QUEEN]  = 'q';
    piece_type['k'] = KING;   piece['k'] = BLACK_KING;   piece[BLACK_KING]   = 'k';

    piece_type['P'] = PAWN;   piece['P'] = WHITE_PAWN;   piece[WHITE_PAWN]   = 'P';
    piece_type['N'] = KNIGHT; piece['N'] = WHITE_KNIGHT; piece[WHITE_KNIGHT] = 'N';
    piece_type['B'] = BISHOP; piece['B'] = WHITE_BISHOP; piece[WHITE_BISHOP] = 'B';
    piece_type['R'] = ROOK;   piece['R'] = WHITE_ROOK;   piece[WHITE_ROOK]   = 'R';
    piece_type['Q'] = QUEEN;  piece['Q'] = WHITE_QUEEN;  piece[WHITE_QUEEN]  = 'Q';
    piece_type['K'] = KING;   piece['K'] = WHITE_KING;   piece[WHITE_KING]   = 'K';
}

/// @brief Parses and initialize the board from a given fen
/// @tparam Magic the type of magic the move generator is using, see config.h
/// @param fen fen to parse
template <typename Magic>
void Board<Magic>::initialize_board(string fen){
    memset(board, 0, 12*sizeof(Bitboard));
    u8 pos = 0, amount, col = 255, board_pos = 0;
    curr_player = NO_COLOR;
    castling_rights = NO_CASTLING;
    curr_turn = 0;
    en_passant = 255;
    char c = fen[pos];
    while(c != ' ' && c != '\0'){
        if(in_range(c-48, 1, 8)){
            board_pos += (c-48);
        }else if((in_range(c-65, 1, 17) || in_range((c^32)-65, 1, 17))){
            board[piece[int(c)]-1] |= 1ULL << (board_pos);
            board_pos++;
        }
        pos++;
        c = fen[pos];
    }
    u8 bar_count = 0;
    while(c != '\0'){
        if(c == 'w'){
            curr_player = WHITE;
        }else if(c == 'b' && curr_player == NO_COLOR){
            curr_player = BLACK;
        }else if(c == 'K'){
            castling_rights |= WHITE_OO;
        }else if(c == 'Q'){
            castling_rights |= WHITE_OOO;
        }else if(c == 'k'){
            castling_rights |= BLACK_OO;
        }else if(c == 'q'){
            castling_rights |= BLACK_OOO;
        }else if(in_range(c-'a', 0, 7)){
            col = c-'a';
        }else if(in_range(c-'0', 0, 9)){
            amount = c-'0';
            if((bar_count == 0 || (bar_count == 1 && col != 255)) && !is_en_passant(en_passant) && in_range(amount, 1, 8)){
                set_en_passant(en_passant, 8-amount, col);
            }else{
                curr_turn *= 10;
                curr_turn += amount;
                if(fen[pos+1] == ' '){
                    rule50 = curr_turn;
                    curr_turn = 0;
                }
            }
        }else if(c == '-'){
            bar_count++;
        }
        pos++;
        c = fen[pos];
    }
    Bitboard white_pieces = 0;
    Bitboard black_pieces = 0;
    for(int i = 0; i < 6; i++){
        black_pieces |= board[i];
    }
    for(int i = 6; i < 12; i++){
        white_pieces |= board[i];
    }
    // cout << (white_pieces) << ' ';
    // cout << (black_pieces) << ' ';
    // cout << (board[11] | board[5]) << ' ';
    // cout << (board[10] | board[4]) << ' ';
    // cout << (board[9]  | board[3]) << ' ';
    // cout << (board[8]  | board[2]) << ' ';
    // cout << (board[7]  | board[1]) << ' ';
    // cout << (board[6]  | board[0]) << '\n';
    // cout << bswap(white_pieces) << ' ';
    // cout << bswap(black_pieces) << ' ';
    // cout << bswap(board[11] | board[5]) << ' ';
    // cout << bswap(board[10] | board[4]) << ' ';
    // cout << bswap(board[9]  | board[3]) << ' ';
    // cout << bswap(board[8]  | board[2]) << ' ';
    // cout << bswap(board[7]  | board[1]) << ' ';
    // cout << bswap(board[6]  | board[0]) << '\n';
}

/// @brief Print the board to stdout
/// @tparam Magic the type of magic the move generator is using, see config.h
/// @param print_extra print extra info of the position (en passant square, current turn, etc.)
/// @param print_bb print the bitboard array
template <typename Magic>
void Board<Magic>::print_board(bool print_extra, bool print_bb){
    u8 b[64];
    memset(b, 0, 64*sizeof(u8));
    for(int k = NO_PIECE; k < WHITE_KING; k++){
        if(print_bb){
            cout << piece[k+1] << '\n';
            print_bitboard(board[k]);
            cout << '\n';
        }
        for(int i = 0; i < 8; i++){
            for(int j = 0; j < 8; j++){
                if((board[k] >> (i*8+j)) & 1) b[i*8+j] = k+1;
            }
        }
    }
    for(int i = 0; i < 8; i++){
        for(int j = 0; j < 8; j++){
            printf("%c ", char(piece[b[i*8+j] & 15]));
        }
        printf("\n");
    }
    if(print_extra){
        printf("Current player: %c\nCurrent turn: %d\nCastling rights: %s\nEn passant: %s\n",
            (curr_player ? 'w' : 'b'), curr_turn, get_castling_rights(castling_rights).c_str(),
            get_square(7-get_en_passant_col(en_passant), get_en_passant_row(en_passant), !is_en_passant(en_passant)).c_str()
        );
    }
}

/// @brief Get a string of the board state, showing the board, current player, current turn, rule 50, castling rights and en passant square
/// @tparam Magic the type of magic the move generator is using, see config.h
/// @return string containing the board state
template <typename Magic>
string Board<Magic>::get_board(){
    u8 b[64];
    memset(b, 0, 64*sizeof(u8));
    for(int k = NO_PIECE; k < WHITE_KING; k++){
        for(int i = 0; i < 8; i++){
            for(int j = 0; j < 8; j++){
                if((board[k] >> (i*8+j)) & 1) b[i*8+j] = k+1;
            }
        }
    }
    string out = "   a b c d e f g h\n";
    for(int i = 0; i < 8; i++){
        out += char(8-i+'0');
        out += "  ";
        for(int j = 0; j < 8; j++){
            out += char(piece[b[i*8+j] & 15]);
            out += ' ';
        }
        out += ' ';
        out += char(8-i+'0');
        out += '\n';
    }
    out += "   a b c d e f g h\n";
    out += "Current player: ";
    out += (curr_player ? 'w' : 'b');
    out += "\nCurrent turn: ";
    char buffer[5];
    // itoa(curr_turn, buffer, 10);
    sprintf(buffer, "%d", curr_turn);
    out += string(buffer);
    out += "\nRule 50: ";
    sprintf(buffer, "%d", rule50);
    out += string(buffer);
    out += "\nCastling rights: ";
    out += get_castling_rights(castling_rights);
    out += "\nEn passant: ";
    out += get_square(7-get_en_passant_col(en_passant), get_en_passant_row(en_passant), !is_en_passant(en_passant)) + '\n';
    return out;
}

/// @brief Print the board to stdout
/// @tparam Magic the type of magic the move generator is using, see config.h
/// @param print_ranks print the ranks on the board
/// @param print_files print the files on the board
/// @param print_extra print extra info of the position (en passant square, current turn, etc.)
/// @param print_bb print the bitboard array
template <typename Magic>
void Board<Magic>::print_board(bool print_ranks, bool print_files, bool print_extra, bool print_bb){
    u8 b[64];
    memset(b, 0, 64*sizeof(u8));
    for(int k = NO_PIECE; k < WHITE_KING; k++){
        if(print_bb){
            cout << piece[k+1] << '\n';
            print_bitboard(board[k]);
            cout << '\n';
        }
        for(int i = 0; i < 8; i++){
            for(int j = 0; j < 8; j++){
                if((board[k] >> (i*8+j)) & 1) b[i*8+j] = k+1;
            }
        }
    }
    if(print_files){
        printf("   a b c d e f g h\n");
    }
    for(int i = 0; i < 8; i++){
        if(print_ranks){
            printf("%d  ", 8-i);
        }
        for(int j = 0; j < 8; j++){
            printf("%c ", char(piece[b[i*8+j] & 15]));
        }
        if(print_ranks){
            printf(" %d ", 8-i);
        }
        printf("\n");
    }
    if(print_files){
        printf("   a b c d e f g h\n");
    }
    if(print_extra){
        printf("Current player: %c\nCurrent turn: %d\nCastling rights: %s\nEn passant: %s\n",
            (curr_player ? 'w' : 'b'), curr_turn, get_castling_rights(castling_rights).c_str(),
            get_square(7-get_en_passant_col(en_passant), get_en_passant_row(en_passant), !is_en_passant(en_passant)).c_str()
        );
    }
}

/// @brief Check if a given square is being attacked
/// @tparam Magic the type of magic the move generator is using, see config.h
/// @param square square to check
/// @param empty_pieces bitboard of the empty squares
/// @return true if the given square is being attacked, false otherwise
template <typename Magic>
bool Board<Magic>::is_square_attacked(u8 square, Bitboard empty_pieces){
    return move_generator->is_square_attacked(square, board, empty_pieces, curr_player);
}

/// @brief Check if a given square is being attacked
/// @tparam Magic the type of magic the move generator is using, see config.h
/// @param square square to check
/// @param board bitboard array representing a position
/// @param empty_pieces bitboard of the empty squares
/// @param color color of the player to check
/// @return true if the given square is being attacked, false otherwise
template <typename Magic>
bool Board<Magic>::is_square_attacked(u8 square, Bitboard board[], Bitboard empty_pieces, Color color){
    return move_generator->is_square_attacked(square, board, empty_pieces, color);
}

/// @brief Check if the current player is in check
/// @tparam Magic the type of magic the move generator is using, see config.h
/// @param empty_pieces bitboard of the empty squares
/// @return true if the current player is in check, false otherwise
template <typename Magic>
bool Board<Magic>::in_check(Bitboard empty_pieces){
    return move_generator->is_square_attacked(__builtin_ctzll(board[(curr_player ? WHITE_KING-1 : BLACK_KING-1)]), board, empty_pieces, curr_player);
}

/// @brief Check if a player is in check
/// @tparam Magic the type of magic the move generator is using, see config.h
/// @param board bitboard array representing the board
/// @param empty_pieces bitboard of the empty squares
/// @param color player to check if in check
/// @return true if the player is in check, false otherwise
template <typename Magic>
bool Board<Magic>::in_check(Bitboard board[], Bitboard empty_pieces, Color color){
    return move_generator->is_square_attacked(__builtin_ctzll(board[(color ? WHITE_KING-1 : BLACK_KING-1)]), board, empty_pieces, color);
}

/// @brief Make a given move in the given board
/// @tparam Magic the type of magic the move generator is using, see config.h
/// @param board bitboard array representing the board
/// @param move move to make
/// @param color player doing the move
/// @param crs castling rights of the board
/// @param eps en passant square
template <typename Magic>
void Board<Magic>::do_move(Bitboard board[], Move move, Color color, CastlingRights &crs, u8 &eps){
    if(move.from == 255 && move.to == 255) return;
    CastlingRights oo = (color ? WHITE_OO : BLACK_OO), ooo = (color ? WHITE_OOO : BLACK_OOO);

    if(move.piece == (color ? WHITE_PAWN-1 : BLACK_PAWN-1) && abs(move.to-move.from) == 16){
        eps = move.to+(color ? 8 : -8);
    }else{
        eps = 255;
    }
    if(move.piece == (color ? WHITE_KING-1 : BLACK_KING-1)){
        crs = CastlingRights(crs & ~(color ? WHITE_CASTLING : BLACK_CASTLING));
    }
    if((crs & oo) && move.piece == (color ? WHITE_ROOK-1 : BLACK_ROOK-1) && move.from == (color ? 63 : 7)){
        crs = CastlingRights(crs & ~oo);
    }
    if((crs & ooo) && move.piece == (color ? WHITE_ROOK-1 : BLACK_ROOK-1) && move.from == (color ? 56 : 0)){
        crs = CastlingRights(crs & ~ooo);
    }
    if(move.capture_piece == (ROOK-1+(color^1)*6)){
        if(move.to == (color ? 7 : 63)){
            crs = CastlingRights(crs & ~(shift(oo, (color ? 2 : -2))));
        }else if(move.to == (color ? 0 : 56)){
            crs = CastlingRights(crs & ~(shift(ooo, (color ? 2 : -2))));
        }
    }
    if(in_range(move.get_en_passant(), 16, 47)){
        board[move.piece] &= ~(1ULL << move.from);
        board[move.piece] |= (1ULL << move.to);

        board[move.capture_piece] &= ~(1ULL << (move.get_en_passant()+(color ? 8 : -8)));
    }else if(move.capture_piece != 255 && (move.promotion_piece == 0 || move.promotion_piece == 255)){
        board[move.piece] &= ~(1ULL << move.from);
        board[move.piece] |= (1ULL << move.to);

        board[move.capture_piece] &= ~(1ULL << move.to);
    }else if(move.promotion_piece != 255){
        board[move.piece] &= ~(1ULL << move.from);
        board[move.promotion_piece+(color*6)] |= (1ULL << move.to);

        if(move.capture_piece != 255){
            board[move.capture_piece] &= ~(1ULL << move.to);
        }
    }else if(move.get_castling() == 1){
        board[move.piece] &= ~(1ULL << move.from);
        board[move.piece] |= (1ULL << move.to);

        board[(color ? WHITE_ROOK-1 : BLACK_ROOK-1)] &= ~(FILE_H & (color ? RANK_1 : RANK_8));
        board[(color ? WHITE_ROOK-1 : BLACK_ROOK-1)] |= (FILE_F & (color ? RANK_1 : RANK_8));
        crs = CastlingRights(crs & ~oo);
    }else if(move.get_castling() == 2){
        board[move.piece] &= ~(1ULL << move.from);
        board[move.piece] |= (1ULL << move.to);

        board[(color ? WHITE_ROOK-1 : BLACK_ROOK-1)] &= ~(FILE_A & (color ? RANK_1 : RANK_8));
        board[(color ? WHITE_ROOK-1 : BLACK_ROOK-1)] |= (FILE_D & (color ? RANK_1 : RANK_8));
        crs = CastlingRights(crs & ~ooo);
    }else{
        board[move.piece] &= ~(1ULL << move.from);
        board[move.piece] |= (1ULL << move.to);
    }
}

/// @brief Make a given move on the current board
/// @tparam Magic the type of magic the move generator is using, see config.h
/// @param move move to make
template <typename Magic>
void Board<Magic>::do_move(Move move){
    this->do_move(board, move, curr_player, castling_rights, en_passant);
    if(move.capture_piece != 255 || move.piece == (curr_player*6)){
        rule50 = 0;
    }else{
        rule50++;
    }
    curr_player = Color(curr_player ^ 1);
    curr_turn++;
}

/// @brief Compute the zobrist hash of the current board
/// @tparam Magic the type of magic the move generator is using, see config.h
/// @return zobrist key
template <typename Magic>
u64 Board<Magic>::zob_hash(){
    return zob_key(*zobrist_table, board, curr_player, castling_rights, en_passant, NULL, true);
}

template class Board<PEXT_Magic>;
template class Board<FIXED_Magic>;

}