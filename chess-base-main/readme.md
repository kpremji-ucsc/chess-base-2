# Chess AI with Magic Bitboards and Negamax

## Implementation Overview

For this assignment I tuned a chess engine by adding movement for the rest of the pieces and featured an AI COM opponent by implementing magic bitboards and negamax with alpha-beta pruning.

## Challenges

1. **Magic Bitboard Implementation**: Generating and validating magic numbers for all 64 squares was complex, used pre-computed values from chess programming resources.

2. **State-Based Move Generation**: Had to refactor moves to work with state strings instead of the Grid for performance during search.

4. **Move Generation Correctness**: Ensuring all piece types generate legal moves correctly, especially pawns (double moves, captures) and handling board edges.

## AI Performance

**Depth Achieved**: 3 (default), easily configurable to 5+

**Search Performance**:
- Depth 3: 10,000+ positions (output of this program)
- Depth 5: 100k positions

**Playing Strength**:
- Makes tactically sound moves
- Captures hanging pieces
- Avoids obvious blunders