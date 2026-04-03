#include <dmsdk/sdk.h>
#include <dmsdk/dlib/math.h>
#include <dmsdk/hid/hid.h>
#include "dump.h"
#include <dmsdk/dlib/time.h>
#include <dmsdk/dlib/profile.h>
#include <stdlib.h> // realloc

#define MODULE_NAME Descend
#define LIB_NAME "descend"


namespace dmDescend
{

struct DescendContext
{
    char*               m_Buffer;
    uint32_t            m_BufferSize;
    uint32_t            m_BufferCapacity;
    bool                m_Initialized;

    dmGameObject::HRegister m_Register;
} g_Descend;

static int Descend_GetForest(lua_State* L)
{
    DM_PROFILE("descend.get_forest");
    
    DM_LUA_STACK_CHECK(L, 1);

    dmDescend::DumpForestToLuaTable(L, g_Descend.m_Register);
    
    return 1;
}

static int Descend_GeTree(lua_State* L)
{
    DM_PROFILE("descend.get_tree");
    
    DM_LUA_STACK_CHECK(L, 1);

    dmDescend::DumpTreeToLuaTable(L, g_Descend.m_Register);

    return 1;
}

// Functions exposed to Lua
static const luaL_reg Descend_module_methods[] =
{
    {"get_forest", Descend_GetForest},
    {"get_tree", Descend_GeTree},
    {0, 0}
};

static void LuaInit(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    luaL_register(L, LIB_NAME, Descend_module_methods);
    lua_pop(L, 1);
}


static dmExtension::Result AppInitialize(dmExtension::AppParams* params)
{
    memset(&g_Descend, 0, sizeof(g_Descend));

    g_Descend.m_Register = dmEngine::GetGameObjectRegister(params);
    g_Descend.m_Initialized = true;

    return dmExtension::RESULT_OK;
}


static dmExtension::Result Initialize(dmExtension::Params* params)
{
    if (!g_Descend.m_Initialized)
        return dmExtension::RESULT_OK;

    LuaInit(params->m_L);

    #if !defined(DM_RELEASE)
    dmLogInfo(" === Registered %s extension", LIB_NAME);
    #endif
    return dmExtension::RESULT_OK;
}

static dmExtension::Result AppFinalize(dmExtension::AppParams* params)
{
    return dmExtension::RESULT_OK;
}

static dmExtension::Result Finalize(dmExtension::Params* params)
{
    return dmExtension::RESULT_OK;
}


}

DM_DECLARE_EXTENSION(MODULE_NAME, LIB_NAME, dmDescend::AppInitialize, dmDescend::AppFinalize, dmDescend::Initialize, 0, 0, dmDescend::Finalize)

#undef LIB_NAME
