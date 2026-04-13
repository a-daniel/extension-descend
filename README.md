# extension-descend
[Defold](https://www.defold.com/) native extension. Returns descendants of game objects.

## Disclaimers
⚠️ Extension Descend relies on Defold's private data structures so it's compatible with a limited number of Defold versions! Currently, it works with Defold 1.12.0 to 1.12.3 !.

## Installation steps
1. Add the repo archive URL https://github.com/a-daniel/extension-descend/archive/refs/heads/main.zip to your `game.project` dependencies.  
2.  - If you don't have a `hooks.editor_script` in your project then you must use the one from this extension. Copy it in the folder of your `game.project` file.  
    - If you already have a `hooks.editor_script` in your project then you must update it to include the functionality from this extension's `hooks.editor_script`.
3. Reload editor scripts (ctrl + shift + R).  

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
