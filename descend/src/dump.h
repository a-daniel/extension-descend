#pragma once

#include <dmsdk/sdk.h>

namespace dmGameObject
{
    /// Component type and game object register
    typedef struct Register* HRegister;
}

namespace dmDescend
{
    void DumpForestToLuaTable(lua_State* L, dmGameObject::HRegister regist);
    
    void DumpTreeToLuaTable(lua_State* L, dmGameObject::HRegister regist);
}


