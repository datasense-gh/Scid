
//////////////////////////////////////////////////////////////////////
//
//  FILE:       engine.h
//              Engine class
//
//  Part of:    Scid (Shane's Chess Information Database)
//  Version:    3.5
//
//  Notice:     Copyright (c) 2002-2003 Shane Hudson.  All rights reserved.
//
//  Author:     Shane Hudson (sgh@users.sourceforge.net)
//
//////////////////////////////////////////////////////////////////////

// The Engine class provides a simple chess position evaluator
// based on negamax with quiescent search and alpha/beta pruning.
// It is used in Scid for doing small quick searches to determine
// which of the possible legal moves to or from a particular square
// to suggest as the best move for faster mouse input.

#ifndef SCID_ENGINE_H
#define SCID_ENGINE_H

#include <stdarg.h>

#include "position.h"
#include "timer.h"

const uint ENGINE_MAX_PLY =           40;  // Maximum search ply.
const int  ENGINE_MAX_HISTORY =   100000;  // Max accumulated history value.
const int  ENGINE_HASH_SCORE = 100000000;  // To order hash moves first.
const uint ENGINE_HASH_KB =           32;  // Default hash table size in KB.
const uint ENGINE_PAWN_KB =            1;  // Default pawn table size in KB.

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// principalVarT
//   Stores the principal variation at one search Ply depth.
//
struct principalVarT {
    uint length;
    ScoredMove move [ENGINE_MAX_PLY];
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// scoreFlagT
//  Types of transposition table score and endgame recognition score.
//
typedef byte scoreFlagT;
const scoreFlagT
    SCORE_NONE  = 0,    // Not a useful score.
    SCORE_EXACT = 1,    // Exact score.
    SCORE_LOWER = 2,    // Lower bound, real score could be higher.
    SCORE_UPPER = 3;    // Upper bound, real score could be lower.

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// transTableEntryT
//   Transposition table entry.
//   Apart from the type flag, depth and score, it also stores the
//   hash codes and other position values for safety checks to avoid
//   a false hit.
//   The best move is also stored, in a compact format to save space.
//
struct transTableEntryT {
    uint    hash;              // Hash value.
    uint    pawnhash;          // Pawn hash value, for extra safety check.
    short   score;             // Evaluation score.
    ushort  bestMove;          // Best move from/to/promote values.
    byte    depth;             // Depth of evaluation.
    byte    flags;             // Score type, side to move and castling flags.
    byte    sequence;          // Sequence number, for detecting old entries.
    squareT enpassant;         // En passant target square.
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// pawnTableEntryT
//   Pawn structure score hash table entry.
//
struct pawnTableEntryT {
    uint  pawnhash;           // Pawn hash value for this pawn structure.
    uint  sig;                // Safety check value, to avoid false hits.
    short score;              // Positional score for pawn structure.
    short wLongbShortScore;   // Pawn storm score for wk on abc, bk on abc.
    short wShortbLongScore;   // Pawn storm score for wk on fgh, bk on fgh.
    byte  fyleHasPassers[2];  // One bit per file, indicating passed pawns.
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// repeatT
//   Repetition-detection stack entry.
//   An entry is pushed onto the stack when a move is made, and
//   popped off when the move is unmade.
//
struct repeatT {
    uint   hash;         // Position hash code.
    uint   pawnhash;     // Position pawn-structure hash code.
    uint   npieces;      // Total number of pieces in position.
    colorT stm;          // Side to move.
};


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Engine
//   Class representing a chess engine.
//
class Engine {
private:
    Position RootPos;       // Position at start of search.
    Position Pos;           // Current position in search.
    uint     MaxDepth;      // Search depth limit.
    int      SearchTime;    // Search time limit in milliseconds.
    int      MinSearchTime; // Minimum search time in milliseconds.
    int      MaxSearchTime; // Maximum search time in milliseconds.
    uint     MinDepthCheckTime; // will not check time before this depth is reached
    bool     Debug;         // If true, print debug info to stdout.
    bool     PostInfo;      // If true, print post info to stdout.
    bool     XBoardMode;    // If true, print info in xboard format.
    bool     Pruning;       // If true, do futility pruning.
#ifndef WINCE
    FILE *   LogFile;       // Output is to stdout and to this file.
#endif
    uint     QNodeCount;    // Nodes examined in quiescent search.
    uint     NodeCount;     // Nodes examined in total.
    Timer    Elapsed;       // Timer for interrupting search.
    bool     IsOutOfTime;   // Becomes true when search is out of time.
    uint     Ply;           // Current ply being examined.
    bool     EasyMove;      // True if the search indicates one move is
                            //    far better than the others.
    bool     HardMove;      // True if failed low at root on current depth.
    uint     InNullMove;    // If > 0, in null move search so no PV updates.
    uint     RepStackSize;         // Repetition stack size.
    repeatT  RepStack [1024];      // Repetition stack.
    bool     InCheck [ENGINE_MAX_PLY];   // In-check at each ply.
    principalVarT PV [ENGINE_MAX_PLY] = {};   // Principal variation at each ply.
    ScoredMove KillerMove [ENGINE_MAX_PLY][2] = {};  // Two killer moves per ply.
    int History[16][64];    // Success history of piece-to-square moves.
    byte     TranTableSequence;    // Transposition table sequence number.
    uint     TranTableSize;        // Number of Transposition table entries.
    transTableEntryT * TranTable;  // Transposition table.
    uint     PawnTableSize;        // Number of Pawn structure table entries.
    pawnTableEntryT * PawnTable;   // Pawn structure score hash table.
    bool (*CallbackFunction)(Engine *, void *);  // Periodic callback.
    void *   CallbackData;

private:
    int PieceValue (pieceT piece);
    int SearchRoot (int depth, int alpha, int beta, MoveList * mlist);
    int Search (int depth, int alpha, int beta, bool tryNullMove);
    int Quiesce (int alpha, int beta);
    int SEE (squareT from, squareT to);
    void ScoreMoves (MoveList * mlist);
    inline void DoMove (ScoredMove * sm);
    inline void UndoMove (ScoredMove * sm);
    inline void SetPVLength (void);
    inline void UpdatePV (ScoredMove * sm);
    void Output (const char * format, ...);
    void PrintPV (uint depth, int score) { PrintPV (depth, score, ""); }
    void PrintPV (uint depth, int score, const char * annotation);
    inline void PushRepeat (Position * pos);
    inline void PopRepeat (void);
    void StoreHash (int depth, scoreFlagT flag, int score,
                    ScoredMove * bestmove, bool isOnlyMove);
    scoreFlagT ProbeHash (int depth, int * score, ScoredMove * bestMove, bool * isOnlyMove);

    inline void ClearKillerMoves (void);
    inline void AddKillerMove (ScoredMove * sm);
    inline bool IsKillerMove (ScoredMove * sm);

    inline void ClearHistoryValues (void);
    inline void HalveHistoryValues (void);
    inline void IncHistoryValue (ScoredMove * sm, int increment);
    inline int GetHistoryValue (ScoredMove * sm);

    int Score (int alpha, int beta);
    inline int ScoreWhiteMaterial (void);
    inline int ScoreBlackMaterial (void);
    void ScorePawnStructure (pawnTableEntryT * pawnEntry);
    bool IsMatingScore (int score);
    bool IsGettingMatedScore (int score);

    bool OutOfTime (void);
    void AdjustTime (bool easyMove);

public:
    Engine()   {
        MaxDepth = ENGINE_MAX_PLY;      // A large default search depth
        SearchTime = 1000;  // Default search time: 1000 ms = one second.
        MinSearchTime = MaxSearchTime = SearchTime;
        MinDepthCheckTime = 4; // will not check time until depth is at least of this value
#ifndef WINCE
        LogFile = NULL;
#endif
        Debug = false;
        PostInfo = false;
        XBoardMode = false;
        Pruning = false;
        RepStackSize = 0;
        TranTable = NULL;
        TranTableSize = 0;
        TranTableSequence = 0;
        PawnTable = NULL;
        PawnTableSize = 0;
        SetHashTableKilobytes (ENGINE_HASH_KB);
        SetPawnTableKilobytes (ENGINE_PAWN_KB);
        CallbackFunction = NULL;
        RootPos.StdStart();
        Pos.StdStart();
        for (auto& e : PV) { e.length = 0; }
    }
#ifdef WINCE
    ~Engine()  { my_Tcl_Free((char*) TranTable);  my_Tcl_Free((char*) PawnTable); }
#else
    ~Engine()  { delete[] TranTable;  delete[] PawnTable; }
#endif
    void SetSearchDepth (uint ply) {
        if (ply < 1) { ply = 1; }
        if (ply > ENGINE_MAX_PLY) { ply = ENGINE_MAX_PLY; }
        MaxDepth = ply;
    }
    void SetSearchTime (uint ms) {
        MinSearchTime = SearchTime = MaxSearchTime = ms;
    }
    void SetSearchTime (uint min, uint ms, uint max) {
        MinSearchTime = min;
        SearchTime = ms;
        MaxSearchTime = max;
    }
    void SetMinDepthCheckTime(uint depth) {
      MinDepthCheckTime = depth;
    }
    void SetDebug (bool b) { Debug = b; }
    void SetPostMode (bool b) { PostInfo = b; }
    bool InPostMode (void) { return PostInfo; }
    void SetXBoardMode (bool b) { XBoardMode = b; }
    bool InXBoardMode (void) { return XBoardMode; }
    void SetPruning (bool b) { Pruning = b; }
#ifndef WINCE
    void SetLogFile (FILE * fp) { LogFile = fp; }
#endif
    void SetHashTableKilobytes (uint sizeKB);
    void SetPawnTableKilobytes (uint sizeKB);
    uint NumHashTableEntries (void) { return TranTableSize; }
    uint NumPawnTableEntries (void) { return PawnTableSize; }
    void ClearHashTable (void);
    void ClearPawnTable (void);
    void ClearHashTables (void) {
        ClearHashTable();
        ClearPawnTable();
    }

    void SetCallbackFunction (bool (*fn)(Engine *, void *), void * data) {
        CallbackFunction = fn;
        CallbackData = data;
    }

    uint GetNodeCount (void) { return NodeCount; }

    bool NoMatingMaterial (void);
    bool FiftyMoveDraw (void);
    uint RepeatedPosition (void);

    void SetPosition (Position * pos);
    Position * GetPosition (void) { return &RootPos; }
    int Score (void);
    int ScoreMaterial (void);
    principalVarT * GetPV (void) { return &(PV[0]); }
    uint PerfTest (uint depth);
    uint ElapsedTime (void) { return Elapsed.MilliSecs(); }
    int Think (MoveList * mlist);
};


inline void
Engine::SetPVLength (void)
{
    if (Ply < ENGINE_MAX_PLY - 1)  {PV[Ply].length = Ply; }
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Engine::UpdatePV
//   Updates the principal variation at the current Ply to
//   include the specified move.
inline void
Engine::UpdatePV (ScoredMove * sm)
{
    if (Ply >= ENGINE_MAX_PLY - 1) { return; }
    if (InNullMove > 0) { return; }
    // if (! Pos.IsLegalMove (sm)) { return; }

    PV[Ply].move[Ply] = *sm;
    for (uint j = Ply + 1; j < PV[Ply + 1].length; j++) {
        PV[Ply].move[j] = PV[Ply+1].move[j];
    }
    PV[Ply].length = PV[Ply+1].length;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Killer moves:
//   We keep track of two "killer" moves at each ply, moves which
//   are not captures or promotions (as they get ordered first) but
//   were good enough to cause a beta cutoff. Killer moves get
//   ordered after good captures but before non-killer noncaptures,
//   which are ordered using the history table (see below).
//
//   Only noncaptures and non-promotion moves can be killer moves, but
//   we make an exception for those that have a negative score (meaning
//   they lose material according to the static exchange evaluator),
//   since they would otherwise be searched last after all noncaptures.

inline void
Engine::ClearKillerMoves (void)
{
    for (uint i=0; i < ENGINE_MAX_PLY; i++) {
        KillerMove[i][0].from = NULL_SQUARE;
        KillerMove[i][1].from = NULL_SQUARE;
    }
}

inline void
Engine::AddKillerMove (ScoredMove* sm)
{
    if (sm->capturedPiece != EMPTY  &&  sm->score >= 0) { return; }
    if (sm->promote != EMPTY  &&  sm->score >= 0) { return; }
    auto killer0 = &(KillerMove[Ply][0]);
    auto killer1 = &(KillerMove[Ply][1]);
    if (killer0->from == sm->from  &&  killer0->to == sm->to
          &&  killer0->movingPiece == sm->movingPiece) {
        return;
    }
    *killer1 = *killer0;
    *killer0 = *sm;
}

inline bool
Engine::IsKillerMove (ScoredMove* sm)
{
    auto killer0 = &(KillerMove[Ply][0]);
    if (killer0->from == sm->from  &&  killer0->to == sm->to
          &&  killer0->movingPiece == sm->movingPiece) {
        return true;        
    }
    auto killer1 = &(KillerMove[Ply][1]);
    if (killer1->from == sm->from  &&  killer1->to == sm->to
          &&  killer1->movingPiece == sm->movingPiece) {
        return true;        
    }
    return false;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// History table:
//   This is a table of values indexed by moving piece and
//   target square, indicating the historical success of each move
//   as measured by the frequency of "good" (better than alpha)
//   scores. It is used to order non-capture moves after killers.

inline void
Engine::ClearHistoryValues (void)
{
    for (pieceT p = WK; p <= BP; p++) {
        for (squareT to = A1; to <= H8; to++) {
            History[p][to] = 0;
        }
    }
}

inline void
Engine::HalveHistoryValues (void)
{
    // Output("# Halving history values\n");
    for (pieceT p = WK; p <= BP; p++) {
        for (squareT to = A1; to <= H8; to++) {
            History[p][to] /= 2;
        }
    }
}

inline void
Engine::IncHistoryValue (ScoredMove * sm, int increment)
{
    if (sm->capturedPiece != EMPTY  &&  sm->score >= 0) { return; }
    if (sm->promote != EMPTY  &&  sm->score >= 0) { return; }
    pieceT p = sm->movingPiece;
    squareT to = sm->to;
    ASSERT (p <= BP  &&  to <= H8);
    History[p][to] += increment;
    // Halve all history values if this one gets too large, to avoid
    // non-capture moves getting searched before captures:
    if (History[p][to] >= ENGINE_MAX_HISTORY) {
        HalveHistoryValues();
    }
}

inline int
Engine::GetHistoryValue (ScoredMove * sm)
{
    pieceT p = sm->movingPiece;
    squareT to = sm->to;
    ASSERT (p <= BP  &&  to <= H8);
    return History[p][to];
}

#endif  // SCID_ENGINE_H

//////////////////////////////////////////////////////////////////////
//  EOF: engine.h
//////////////////////////////////////////////////////////////////////
