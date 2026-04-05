#include "dump.h"
#include "private_dependencies.h"
#include <dmsdk/gameobject/gameobject.h>
#include <dmsdk/dlib/math.h>
#include <dmsdk/script/script.h>
#include <cstring>
#include <vector>
#include <cstddef>

namespace dmDescend
{
    const uint32_t INVALID_INSTANCE_INDEX = 0xffff;
    
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
        }else{
            //push parent node id
            dmScript::PushHash(L, phinstance->m_Identifier); 

            //create and push empty table 
            lua_newtable(L);

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