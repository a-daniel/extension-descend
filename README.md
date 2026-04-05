# extension-descend
[Defold](https://www.defold.com/) native extension. Returns descendants of game objects.

## Disclaimers
⚠️ Extension Descend relies on game engines's private data structures so it's vulnerable to game engine internal changes! Currently known to work with Defold 1.12.2.

## Installation
To use this library in your Defold project add the repo archive URL https://github.com/a-daniel/extension-descend/archive/refs/heads/main.zip to your `game.project` dependencies. 

## API
```lua
descend.get_tree(ancestor_id)
```
Returns a tree of game objects that have a particular ancestor. The tree is represented as a table that maps the id(hash) of a game object to an array of it's children(hash).
```lua
descend.get_forest()
```
 Returns all the trees in the scene. The forest is represented as a table that maps the id(hash) of a root game object to it's tree.


## Usage

```lua
--=== get descendants of a game object
local ancestor_id = go.get_id("/some_go") 
local descendants_tree = descend.get_tree(ancestor_id)

-- ==== traverse descendants of a game object
local function traverse_tree(tree, node)
  if tree and tree[node] then
    print(node) -- process node
    for _,child in ipairs(tree[node]) do
      traverse_tree(tree,child)
    end
  end
end

traverse_tree(descendants_tree, ancestor_id) 

--==== traverse all game objects in the scene
local forest = descend.get_forest()

for ancestor_id,descendants_tree in pairs(forest) do
  traverse_tree(descendants_tree,ancestor_id)
end
```

## Performance concerns
For a scene with 1024 game objects, descend.get_forest takes 0.5-1ms on an AMD FX-9830P.
The time spent in this extension can be seen in the web profiler under the names descent.get_tree and descend.get_forest.
