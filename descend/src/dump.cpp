#include "dump.h"
#include "private_dependencies.h"
#include <dmsdk/gameobject/gameobject.h>
#include <dmsdk/dlib/math.h>
#include <dmsdk/script/script.h>
#include <cstring>
#include <vector>
#include <cstddef>

// checking if game engine changed private data structures we use
static_assert(sizeof(struct dmGameObject::Register) == 53608, "Unexpected struct size. Mismatch between Descend extension and game engine.");
static_assert(sizeof(struct dmGameObject::CollectionHandle) == 8, "Unexpected struct size. Mismatch between Descend extension and game engine.");
static_assert(sizeof(struct dmGameObject::Collection) == 6480, "Unexpected struct size. Mismatch between Descend extension and game engine.");
static_assert(sizeof(class dmGameObject::dmIndexPool16) == 16, "Unexpected struct size. Mismatch between Descend extension and game engine.");
static_assert(sizeof(class dmGameObject::dmIndexPool32) == 24, "Unexpected struct size. Mismatch between Descend extension and game engine.");
static_assert(sizeof(struct dmGameObject::ComponentType) == 208, "Unexpected struct size. Mismatch between Descend extension and game engine.");
static_assert(sizeof(struct dmGameObject::Instance) == 176, "Unexpected struct size. Mismatch between extension Descend and game engine.");

static_assert(offsetof(struct dmGameObject::Register,m_Mutex) == 53560, "Unexpected struct layout. Mismatch between extension Descend and game engine.");
static_assert(offsetof(struct dmGameObject::Register,m_Collections) == 53568, "Unexpected struct layout. Mismatch between extension Descend and game engine.");
static_assert(offsetof(struct dmGameObject::Collection,m_Instances) == 2072, "Unexpected struct layout. Mismatch between extension Descend and game engine.");
static_assert(offsetof(struct dmGameObject::Collection,m_LevelIndices) == 2152, "Unexpected struct layout. Mismatch between extension Descend and game engine.");
static_assert(offsetof(struct dmGameObject::Collection,m_Mutex) == 6424, "Unexpected struct layout. Mismatch between extension Descend and game engine.");
static_assert(offsetof(struct dmGameObject::Instance,m_Collection) == 80, "Unexpected struct layout. Mismatch between extension Descend and game engine.");
static_assert(offsetof(struct dmGameObject::Instance,m_Identifier) == 104, "Unexpected struct layout. Mismatch between extension Descend and game engine.");
static_assert(offsetof(struct dmGameObject::CollectionHandle,m_Collection) == 0, "Unexpected struct layout. Mismatch between extension Descend and game engine.");


namespace dmDescend
{
    const uint32_t INVALID_INSTANCE_INDEX = 0xffff;

    // static_assert(sizeof(MyStruct) == expected, "Size mismatch");
    // static_assert(offsetof(MyStruct, field) == expected_offset, "Layout mismatch");
    
    static void GoSubtreeToLua(lua_State* L, dmGameObject::HInstance phinstance)
    {
        if (phinstance->m_FirstChildIndex != INVALID_INSTANCE_INDEX){
            std::vector<dmhash_t> cids;
            
            // first child
            dmGameObject::HInstance chinstance = phinstance->m_Collection->m_Instances[phinstance->m_FirstChildIndex];
            cids.push_back(chinstance->m_Identifier);
            
            GoSubtreeToLua(L,chinstance);
            
            // first child's siblings
            while(chinstance->m_SiblingIndex != INVALID_INSTANCE_INDEX)
            {
                chinstance = chinstance->m_Collection->m_Instances[chinstance->m_SiblingIndex];
                cids.push_back(chinstance->m_Identifier);

                GoSubtreeToLua(L,chinstance);
            }
 
            //push parent node id
            dmScript::PushHash(L, phinstance->m_Identifier); 

            //create and push empty table 
            lua_newtable(L);

            // save child ids in smaller table
            for (int i = 0; i < cids.size(); ++i) {
                //save children id at index 1+counter
                lua_pushinteger(L, i+1); 
                dmScript::PushHash(L, cids[i]); 
                lua_settable(L, -3); 
            }

            lua_settable(L, -3);
        }
    }

    bool getInstanceFromId(dmGameObject::HRegister regist, dmhash_t id,  dmGameObject::HInstance *hinstance){
        dmGameObject::CollectionHandle hcollection;

        DM_MUTEX_SCOPED_LOCK(regist->m_Mutex);
    
        for (int i = 0; i < regist->m_Collections.Size(); ++i) {
            dmMutex::Lock(regist->m_Collections[i]->m_Mutex);
            hcollection.m_Collection = regist->m_Collections[i];
            *hinstance = GetInstanceFromIdentifier(&hcollection, id);
            dmMutex::Unlock(regist->m_Collections[i]->m_Mutex);
            
            if (*hinstance){
                return true;
            }
        }
        return false;
    }
    
    void DumpForestToLuaTable(lua_State* L, dmGameObject::HRegister regist)
    {
        DM_LUA_STACK_CHECK(L, 1);
        
        lua_newtable(L);
        
        DM_MUTEX_SCOPED_LOCK(regist->m_Mutex);
        for (int ccntr = 0; ccntr < regist->m_Collections.Size(); ++ccntr) {

            dmGameObject::Collection* hcollection = regist->m_Collections[ccntr];
            dmMutex::Lock(hcollection->m_Mutex);
            
            for (int rootcntr = 0; rootcntr < hcollection->m_LevelIndices[0].Size(); ++rootcntr) {
                
                int instance_idx = hcollection->m_LevelIndices[0][rootcntr];
                dmGameObject::Instance * instance = hcollection->m_Instances[instance_idx];
                dmhash_t id = instance->m_Identifier;
                
                //push root node id
                dmScript::PushHash(L, id); 
                //create and push empty table 
                lua_newtable(L);
                GoSubtreeToLua(L, instance);
                lua_settable(L, -3);
            }

            dmMutex::Unlock(hcollection->m_Mutex);
        }
    }
    

    void DumpTreeToLuaTable(lua_State* L, dmGameObject::HRegister regist)
    {
        DM_LUA_STACK_CHECK(L, 1);

        bool res_ok = false;
        if (!lua_isnone(L,1) && !lua_isnil(L,1)){ 
            dmhash_t root_id = dmScript::CheckHashOrString(L, 1); // should accept also URL !?!?
            if (root_id){
                dmGameObject::HInstance hinstance;
                if(getInstanceFromId(regist, root_id, &hinstance)){
                    lua_newtable(L);
                    dmMutex::Lock(hinstance->m_Collection->m_Mutex);
                    GoSubtreeToLua(L, hinstance);
                    dmMutex::Unlock(hinstance->m_Collection->m_Mutex);
                    res_ok = true;
                }
            }
        }

        if (!res_ok){
            lua_pushnil(L); 
            return;
        }
    }
}