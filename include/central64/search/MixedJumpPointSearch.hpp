#pragma once
#ifndef CENTRAL64_MIXED_JUMP_POINT_SEARCH
#define CENTRAL64_MIXED_JUMP_POINT_SEARCH

#include <central64/search/MixedSearch.hpp>

namespace central64 {

/// A derived class for the `L`-neighbor mixed jump point search method with `M`-by-`M` blocks.
template <int L, int M = 8>
class MixedJumpPointSearch : public MixedSearch<L, M>
{
public:
    explicit MixedJumpPointSearch(const Grid2D<L>& grid) : MixedSearch{ grid } {}  ///< Create a mixed jump point search object that references an existing grid object.

    MixedJumpPointSearch(MixedJumpPointSearch&&) = default;             ///< Ensure the default move constructor is public.
    MixedJumpPointSearch& operator=(MixedJumpPointSearch&&) = default;  ///< Ensure the default move assignment operator is public.

    std::string MethodName() const { return "Mixed Jump Point Search"; }                  ///< Get the name of the search method.
    std::string AllNodesMethodName() const { return "Mixed Canonical Dijkstra Search"; }  ///< Get the name of the all-nodes version of the search method.

private:
    // Process the node at coordinates `coords`.
    void ProcessSearchNode(Offset2D coords);

    void Jump(Offset2D initialCoords, Offset2D initialCompressedCoords);
};

template <int L, int M>
void MixedJumpPointSearch<L, M>::ProcessSearchNode(Offset2D coords)
{
    // Simply initiate a jump traversal. There is no need to
    // check termination conditions because all reachable
    // nodes in the current block are to be processed.
    Jump(coords, CompressCoords(coords));
}

template <int L, int M>
void MixedJumpPointSearch<L, M>::Jump(Offset2D initialCoords, Offset2D initialCompressedCoords)
{
    const Move<L>& parentMove = Tree().ParentMove(initialCoords);
    const Move<L> forwardMove = -parentMove;
    Offset2D coords = initialCoords;
    bool jumping = true;
    while (jumping) {
        const Connections<L> canonicalSuccessors = Grid().CanonicalSuccessors(coords, parentMove);
        const Connections<L> forcedSuccessors = Grid().ForcedSuccessors(coords, parentMove);
        Offset2D compressedCoords = CompressCoords(coords);
        if (compressedCoords != initialCompressedCoords) {
            // The jump traversal has entered into a different block.
            // Update the search based on the cost of the border node,
            // and terminate the jump traversal.
            UpdateBorderNode(coords);
            jumping = false;
        }
        else {
            if (!forcedSuccessors.IsAllDisconnected()) {
                // Forced successors have been encountered. Expand
                // the current node and terminate the jump traversal.
                ExpandSearchNode(coords, forcedSuccessors | canonicalSuccessors);
                jumping = false;
            }
            else {
                // The jump traversal is to continue. Get the g-cost of the current node.
                PathCost gCost = Tree().GCost(coords);

                // If the forward direction of the traversal is odd, then initialize and 
                // update any canonical successors that are not in the forward direction.
                // Initiate a jump traversal in these even directions.
                if (forwardMove.IsOdd()) {
                    for (const Move<L>& move : Neighborhood<L>::Moves()) {
                        if (canonicalSuccessors.IsConnected(move)) {
                            if (move != forwardMove) {
                                const Offset2D successorCoords = coords + move.Offset();
                                const Offset2D successorCompressedCoords = CompressCoords(successorCoords);
                                if (!IsSearchBlockInitialized(successorCompressedCoords)) {
                                    InitializeSearchBlock(successorCompressedCoords);
                                }
                                const PathCost successorGCost = gCost + move.Cost();
                                if (successorGCost < Tree().GCost(successorCoords)) {
                                    Tree().SetGCost(successorCoords, successorGCost);
                                    Tree().SetParentMove(successorCoords, -move);
                                    Jump(successorCoords, initialCompressedCoords);
                                }
                            }
                        }
                    }
                }

                // If there is no canonical successor in the forward direction,
                // terminate the jump traversal. Otherwise, advance by one move
                // in the forward direction.
                if (!canonicalSuccessors.IsConnected(forwardMove)) {
                    jumping = false;
                }
                else {
                    // Advance to the next node and ensure the encompassing block is initialized.
                    coords += forwardMove.Offset();
                    compressedCoords = CompressCoords(coords);
                    if (!IsSearchBlockInitialized(compressedCoords)) {
                        InitializeSearchBlock(compressedCoords);
                    }

                    // Obtain the g-cost of a path to the current node from the previous node.
                    // If this is now the shortest path to the current node, update the path tree
                    // and continue the jump traversal.
                    gCost += forwardMove.Cost();
                    jumping = (gCost < Tree().GCost(coords));
                    if (jumping) {
                        Tree().SetGCost(coords, gCost);
                        Tree().SetParentMove(coords, parentMove);
                    }
                }
            }
        }
    }
}

}  // namespace

#endif
