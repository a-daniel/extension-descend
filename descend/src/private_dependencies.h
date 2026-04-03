#pragma once

#include <dmsdk/dlib/hash.h>
#include <dmsdk/dlib/vmath.h>
#include <dmsdk/dlib/transform.h>

// !!!!!!!! improvization needed because we can't include gameobject_private.h  !!!!!!!!
#warning (Extension Descend violates strict aliasing rule! This may lead to undefined behaviour!)
#warning (Extension Descend relies on game engines`s private data structures so it`s vulnerable to private changes!)

namespace dmGameObject
{
    // =====  copy of struct Instance from gameobject_private.h 
    struct Instance
    {
        Instance(void* prototype)
        {
        }
        ~Instance()
        {
        }
        dmTransform::Transform m_Transform;
        // Shadowed rotation expressed in euler coordinates
        dmVMath::Vector3 m_EulerRotation;
        // Previous euler rotation, used to detect if the euler rotation has changed and should overwrite the real rotation (needed by animation)
        dmVMath::Vector3 m_PrevEulerRotation;
        // Collection this instances belongs to. Added for GetWorldPosition.
        // We should consider to remove this (memory footprint)
        struct Collection* m_Collection;
        void*      m_Prototype;
        uint32_t        m_IdentifierIndex;
        dmhash_t        m_Identifier;
        // Collection path hash-state. Used for calculating global identifiers. Contains the hash-state for the collection-path to the instance.
        // We might, in the future, for memory reasons, move this hash-state to a data-structure shared among all instances from the same collection.
        HashState64     m_CollectionPathHashState;
        // Hierarchical depth
        uint16_t        m_Depth : 8;
        // If the instance was initialized or not (Init())
        uint16_t        m_Initialized : 1;
        // If this game object is part of a skeleton
        uint16_t        m_Bone : 1;
        // If this is a generated instance, i.e. if the instance id is uniquely generated
        uint16_t        m_Generated : 1;
        // Used for deferred deletion
        uint16_t        m_ToBeDeleted : 1;
        // Used for deferred add-to-update
        uint16_t        m_ToBeAdded : 1;
        // Padding
        uint16_t        m_Pad : 3;
        // Index to parent
        uint16_t        m_Parent : 16;
        // Index to Collection::m_Instances
        uint16_t        m_Index : 16;
        // Index to Collection::m_LevelIndex. Index is relative to current level (m_Depth), eg first object in level L always has level-index 0
        // Level-index is used to reorder Collection::m_LevelIndex entries in O(1). Given an instance we need to find where the
        // instance index is located in Collection::m_LevelIndex
        uint16_t        m_LevelIndex : 16;
        // Index to next instance to delete or INVALID_INSTANCE_INDEX
        uint16_t        m_NextToDelete : 16;
        // Index to next instance to add-to-update or INVALID_INSTANCE_INDEX
        uint16_t        m_NextToAdd;
        // Next sibling index. Index to Collection::m_Instances
        uint16_t        m_SiblingIndex : 16;
        // First child index. Index to Collection::m_Instances
        uint16_t        m_FirstChildIndex : 16;
        uint32_t        m_ComponentInstanceUserDataCount;
        uintptr_t       m_ComponentInstanceUserData[0];
    };

    struct ComponentType
    {
        ComponentType();

        void*           m_ResourceType;
        const char*             m_Name;
        dmhash_t                m_NameHash;
        void*                   m_Context;
        void*       m_NewWorldFunction;
        void*    m_DeleteWorldFunction;
        void*         m_CreateFunction;
        void*        m_DestroyFunction;
        void*           m_InitFunction;
        void*          m_FinalFunction;
        void*    m_AddToUpdateFunction;
        void*            m_GetFunction;
        void*        m_FixedUpdateFunction;
        void*        m_UpdateFunction;
        void*        m_LateUpdateFunction;
        void*        m_RenderFunction;
        void*    m_PostUpdateFunction;
        void*      m_OnMessageFunction;
        void*        m_OnInputFunction;
        void*       m_OnReloadFunction;
        void*  m_SetPropertiesFunction;
        void*    m_GetPropertyFunction;
        void*    m_SetPropertyFunction;
        void*       m_IterChildren; // for debug/testing
        void*     m_IterProperties; // for debug/testing
        uint32_t                m_TypeIndex : 16;
        uint32_t                m_InstanceHasUserData : 1;
        uint32_t                m_ReadsTransforms : 1;
        uint32_t                m_Reserved : 14;
        uint16_t                m_UpdateOrderPrio;
    };

    template <typename T>
    class dmIndexPool
    {
        enum STATE_FLAGS
        {
            STATE_DEFAULT           = 0x0,
            STATE_USER_ALLOCATED    = 0x1
        };

        public:
        /**
        * Creates an empty index pool.
        */
        dmIndexPool();

        template <typename CONTEXT>
        void IterateRemaining(void (*call_back)(CONTEXT *context, T index), CONTEXT* context);

        private:
        T*   m_Pool;
        T    m_Capacity;
        T    m_Size;
        uint16_t m_State : 1;
    };
    class dmIndexPool16 : public dmIndexPool<uint16_t> {};
    class dmIndexPool32 : public dmIndexPool<uint32_t> {};

    const uint32_t MAX_HIERARCHICAL_DEPTH = 128;
    struct Collection
    {
        Collection();
        // Resource factory
        void*     m_Factory;
        // GameObject component register
        void*                m_Register;
        // dmGameObject::HCollection m_HCollection;
        void* m_HCollection;
        // Component type specific worlds
        void*                    m_ComponentWorlds[255];
        // Maximum number of instances
        uint32_t                 m_MaxInstances;
        // Array of instances. Zero values for free slots. Order must
        // always be preserved. Slots are allocated using index-pool
        // m_InstanceIndices below
        // Size if always = max_instances (at least for now)
        dmArray<Instance*>       m_Instances;
        // Index pool for mapping Instance::m_Index to m_Instances
        dmIndexPool16            m_InstanceIndices;
        // Resources referenced through property overrides inside the collection
        dmArray<void*>           m_PropertyResources;
        // Array of dynamically allocated index arrays, one for each level
        // Used for calculating transforms in scene-graph
        // Two dimensional table of indices with stride "max_instances"
        // Level 0 contains root-nodes in [0..m_LevelIndices[0].Size()-1]
        // Level 1 contains level 1 indices in [0..m_LevelIndices[1].Size()-1]
        dmArray<uint16_t>        m_LevelIndices[MAX_HIERARCHICAL_DEPTH];
        // Array of world transforms. Calculated using m_LevelIndices above
        dmArray<dmVMath::Matrix4>         m_WorldTransforms;
        // Identifier to Instance mapping
        dmHashTable64<void*> m_IDToInstance;
        // Stack keeping track of which instance has the input focus
        dmArray<void*>       m_InputFocusStack;
        // Array of dynamically created resources (i.e runtime-only resources)
        dmArray<dmhash_t>        m_DynamicResources;
        // Name-hash of the collection.
        dmhash_t                 m_NameHash;
        // Socket for sending to instances, dispatched between every component update
        void*       m_ComponentSocket;
        // Socket for sending to instances, dispatched once each update
        void*       m_FrameSocket;
        dmMutex::HMutex          m_Mutex;
        // Counter for generating instance ids, protected by m_Mutex
        uint32_t                 m_GenInstanceCounter;
        uint32_t                 m_GenCollectionInstanceCounter;
        dmIndexPool32            m_InstanceIdPool;
        // Head of linked list of instances scheduled for deferred deletion
        uint16_t                 m_InstancesToDeleteHead;
        // Tail of the same list, for O(1) appending
        uint16_t                 m_InstancesToDeleteTail;
        // Head of linked list of instances scheduled to be added to update
        uint16_t                 m_InstancesToAddHead;
        // Tail of the same list, for O(1) appending
        uint16_t                 m_InstancesToAddTail;
        float                    m_FixedAccumTime;  // Accumulated time between fixed updates. Scaled time.
        // Set to 1 if in update-loop
        uint32_t                 m_InUpdate : 1;
        // Used for deferred deletion
        uint32_t                 m_ToBeDeleted : 1;
        uint32_t                 m_DirtyTransforms : 1;
        uint32_t                 m_Initialized : 1;
        uint32_t                 m_FirstUpdate : 1;
    };

    struct CollectionHandle
    {
        Collection* m_Collection;
    };
    
    const uint32_t MAX_COMPONENT_TYPES = 255;
    
    struct Register
    {
        uint32_t                    m_ComponentTypeCount;
        ComponentType               m_ComponentTypes[MAX_COMPONENT_TYPES];
        uint16_t                    m_ComponentTypesOrder[MAX_COMPONENT_TYPES];
        dmMutex::HMutex             m_Mutex;

        // All collections. Protected by m_Mutex
        dmArray<Collection*>        m_Collections;
        // Default capacity of collections
        uint32_t                    m_DefaultCollectionCapacity;
        uint32_t                    m_DefaultInputStackCapacity;

        Register();
        ~Register();
    };
}


