#include <stack>
#include <unordered_set>

#include "loops_search.hh"
#include "graph.hh"

namespace dumb
{
namespace loops
{

LoopsInfo::LoopsInfo( const graph::Graph<ir::BasicBlockID>& cfg)
{
    collect_natural_loops( cfg);
    for ( std::unique_ptr<Loop>& loop : loops_ )
    {
        discover_loop( *loop, cfg);
    }
    build_loops_tree();
    build_blocks_map();
}

void
LoopsInfo::collect_natural_loops( const CfgT& cfg)
{
    const graph::DominatorsTable<ir::BasicBlockID>& dominators = cfg.GetDominatorsTable();

    for ( ir::BasicBlockID node_b : cfg.UsedIds() )
    {
        for ( ir::BasicBlockID node_a : cfg.GetNexts( node_b) )
        {
            // Looking for edge B -> A
            // Backedge if A dominates B
            if ( dominators.Dominates( node_a, node_b) )
            {
                ir::BasicBlockID preheader;

                std::size_t preds_number = cfg.GetPreds( node_a).size();
                if ( preds_number == 1 )
                {
                    preheader = cfg.GetPreds( node_a).back();
                } else
                {
                    if ( preds_number != 2 )
                    {
                        throw std::runtime_error{ "Unexpected to see not natural loop: "
                                                  "expected to see 1 or 2 predecessors for block"};
                    }

                    ir::BasicBlockID first = cfg.GetPreds( node_a).back();
                    ir::BasicBlockID second = cfg.GetPreds( node_a).front();

                    if ( first == node_b )
                    {
                        preheader = second;
                    } else if ( second == node_b )
                    {
                        preheader = first;
                    } else
                    {
                        throw std::runtime_error{ "Unexpected to see not natural loop: "
                                                  "expected to 1 of 2 predecessors to be equal to "
                                                  "latch (end of backedge)"};
                    }
                }

                loops_.emplace_back( std::make_unique<Loop>( preheader, node_a, node_b));
            }
        }
    }
}

void
LoopsInfo::discover_loop( Loop&       loop,
                          const CfgT& cfg)
{
    loop.blocks.insert( loop.header);

    std::stack<ir::BasicBlockID> stack{};
    stack.push( loop.latch);

    while ( !stack.empty() )
    {
        ir::BasicBlockID block = stack.top();
        stack.pop();

        if ( loop.Contains( block) )
        {
            continue;
        }
        loop.blocks.insert( block);

        for ( ir::BasicBlockID pred : cfg.GetPreds( block) )
        {
            if ( pred != loop.header )
            {
                stack.push( pred);
            }
        }
    }
}

void
LoopsInfo::build_loops_tree()
{
    for ( std::unique_ptr<Loop>& current_ptr : loops_ )
    {
        Loop *current = current_ptr.get();

        Loop *best_parent = nullptr;
        for ( std::unique_ptr<Loop>& other_ptr : loops_ )
        {
            Loop *other = other_ptr.get();
            if ( current == other )
            {
                continue;
            }

            if ( !other->Contains( *current) )
            {
                continue;
            }

            if ( (best_parent == nullptr) ||
                 (best_parent->blocks.size() > other->blocks.size()) )
            {
                best_parent = other;
            }
        }
        current->parent = best_parent;
        if ( best_parent != nullptr )
        {
            best_parent->children.emplace_back( current);
            current->depth = best_parent->depth + 1;
        } else
        {
            roots_.emplace_back( current);
        }
    }

    for ( Loop *root : roots_ )
    {
        set_depth_value( *root, 0);
    }
}

void
LoopsInfo::set_depth_value( Loop& loop,
                            int   depth)
{
    loop.depth = depth;
    for ( Loop *child : loop.children )
    {
        set_depth_value( *child, depth + 1);
    }
}

void
LoopsInfo::build_blocks_map()
{
    for ( const std::unique_ptr<Loop>& loop : loops_ )
    {
        for ( ir::BasicBlockID bb : loop->blocks )
        {
            auto it = blocks_map_.find( bb);
            if ( it == blocks_map_.end() )
            {
                // First time for this block
                blocks_map_[bb] = loop.get();
            } else
            {
                // Second time for this block, selecting the loop with bigger depth.
                Loop *current = it->second;
                if ( current->depth < loop->depth )
                {
                    blocks_map_[bb] = loop.get();
                }
            }
        }
    }
}

std::string
LoopsInfo::ToStr() const
{
    return to_str_level( roots_);
}

std::string
LoopsInfo::to_str_level( const std::vector<Loop *> level) const
{
    std::string result = "";

    if ( level.empty() )
    {
        return result;
    }

    std::string padding = "";
    for ( int i = 0; i != level[0]->depth; ++i )
    {
        padding += "  ";
    }
    padding += " | ";

    for ( const Loop *loop : level )
    {
        result += padding;

        result += "(header=" + std::to_string( loop->header) +
                  ", latch=" + std::to_string( loop->latch) +
                  ", depth=" + std::to_string( loop->depth) + "): blocks={";

        auto it = loop->blocks.begin();
        for ( ; std::next( it) != loop->blocks.end(); ++it )
        {
            result += std::to_string( *it) + ", ";
        }
        result += std::to_string( *it) + "}\n";

        result += to_str_level( loop->children);
    }
    return result;
}

} // ! namespace loops
} // ! namespace dumb
