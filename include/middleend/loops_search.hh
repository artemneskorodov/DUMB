#ifndef DUMB_LOOPS_SEARCH_HH__
#define DUMB_LOOPS_SEARCH_HH__

#include <unordered_set>
#include <vector>
#include <memory>

#include "ir.hh"
#include "graph.hh"

namespace dumb
{
namespace loops
{

///
/// @brief Helper structure to hold information about loops in IR. Used in loops tree.
///
struct Loop
{
    Loop( ir::BasicBlockID preheader,
          ir::BasicBlockID header,
          ir::BasicBlockID latch)
     :  preheader { preheader},
        header    { header},
        latch     { latch}
    {
    }

    ir::BasicBlockID                      preheader;
    ir::BasicBlockID                      header;
    ir::BasicBlockID                      latch;

    std::unordered_set<ir::BasicBlockID>  blocks   {};

    Loop                                 *parent   { nullptr};
    std::vector<Loop *>                   children {};
    int                                   depth    { 0};

    bool
    Contains( ir::BasicBlockID block) const
    {
        return blocks.contains( block);
    }

    bool
    IsInnermost() const
    {
        return children.empty();
    }

    bool
    IsBlockDirectlyInLoop( ir::BasicBlockID block) const
    {
        if ( !Contains( block) )
        {
            return false;
        }
        for ( Loop *child : children )
        {
            if ( child->Contains( block) )
            {
                return false;
            }
        }
        return true;
    }


    bool
    Contains( const Loop& other) const
    {
        if ( blocks.size() < other.blocks.size() )
        {
            return false;
        }
        for ( ir::BasicBlockID bb : other.blocks )
        {
            if ( !blocks.contains( bb) )
            {
                return false;
            }
        }
        return true;
    }

};

class LoopsInfo
{
public:
    LoopsInfo( const graph::Graph<ir::BasicBlockID>& cfg);

    Loop *
    GetLoopFor( ir::BasicBlockID id) const
    {
        auto it = blocks_map_.find( id);
        if ( it == blocks_map_.end() )
        {
            return nullptr;
        }
        return it->second;
    }

    bool
    IsLoopHeader( ir::BasicBlockID id) const
    {
        auto it = blocks_map_.find( id);
        if ( it == blocks_map_.end() )
        {
            return false;
        }
        return (it->second->header == id);
    }

    const std::vector<std::unique_ptr<Loop>>&
    Loops() const &
    {
        return loops_;
    }

    std::string
    ToStr() const;

private:
    using CfgT = graph::Graph<ir::BasicBlockID>;

private:
    void collect_natural_loops( const CfgT& cfg);
    void discover_loop( Loop& loop, const CfgT& cfg);
    void build_loops_tree();
    void set_depth_value( Loop& loop, int depth);
    void build_blocks_map();
    std::string to_str_level( const std::vector<Loop *> level) const;

private:
    std::vector<std::unique_ptr<Loop>>           loops_      {};
    std::vector<Loop *>                          roots_      {};
    std::unordered_map<ir::BasicBlockID, Loop *> blocks_map_ {};

};

LoopsInfo GetLoopsTree( const ir::Function& program);

} // ! namespace loops
} // ! namespace dumb

#endif // ! DUMB_LOOPS_SEARCH_HH__
