// --------------------------------------------------------------------------
// Declaration of the UnrealEngine implementation of the PackageMapper API.
//
// Copyright (c) 2018 The Foundry Visionmongers Ltd. All Rights Reserved.
// --------------------------------------------------------------------------
#ifndef PACKAGEMAPPER__H
#define PACKAGEMAPPER__H

#include "Package.h"
#include "Response.h"

#include <map>
#include <string>
#include <vector>
#include <unordered_map>
#include <forward_list>

#include "AssetToolsModule.h"
#include "SceneProtocolUserData.h"

#include "Camera/CameraActor.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/Material.h"

#define MESH_PACKAGE_ROOT "/Game/SceneProtocol/Mesh"
#define MESH_INST_PACKAGE_ROOT "/Game/SceneProtocol/Instance"
#define LIGHTS_PACKAGE_ROOT "/Game/SceneProtocol/Lights"
#define TEXTURES_PACKAGE_ROOT "/Game/SceneProtocol/Textures"
#define MATERIAL_DEF_PACKAGE_ROOT "/Game/SceneProtocol/MaterialDefinitions"
#define MATERIAL_INST_PACKAGE_ROOT "/Game/SceneProtocol/MaterialInstances"
#define CAMERA_PACKAGE_ROOT "/Game/SceneProtocol/Camera"

class FString;
class FName;
class UObject;
class UMaterialInterface;

namespace SceneProtocol {
  namespace NetworkBridge {
    namespace BridgeProtocol {
      class IPackage;

    }

    namespace Unreal {

      class PackageMapper {
      public:
        static PackageMapper&  instance();

        // Iterates through available items and regenerates package maps
        //
        // This method skips over items that don't have custom user data.
        // Items without custom user data haven't been recieved/sent by STP 
        // or user didn't add STP custom user data.
        void generatePackageMaps();

        // Clears package and unique name maps
        void clearMaps();

        // Registers event handlers for events generated by UnrealEngine, e.g. when
        // an actor is destroyed. This allows the AssetFactory to ensure that its
        // internal state matches that of the engine.
        void registerEventHandlers();

        // Generate unique package name
        FString generatePackageName(const FString& packageRoot, const std::string& token);

        bool tokenExists(const std::string& token);

        //
        // Package registration
        //

        // Adds one Unreal UPackage object to the package map by creating a wrapper UnownedPackage object
        // Unowned packages are one Unreal created or loaded and don't use our custom package
        bool registerUnownedPackage(const std::string& token, FName packageName, UPackage* package, std::vector<UObject*>& objects);

        // Adds one owned custom IPackage object to the package map
        // Owned packages are STP created
        bool registerOwnedPackage(const std::string& token, SceneProtocol::NetworkBridge::BridgeProtocol::IPackage* package);

        // Adds package Unreal objects to the map
        // If the package already exists objects mapped to the package will be replaced
        void addPackageObjects(SceneProtocol::NetworkBridge::BridgeProtocol::IPackage* package, std::vector<UObject*>& objects);

        //
        // Package and object retrieval
        //

        // If the package exists in the package maps
        bool hasPackage(FName packageName);
        
        // Find Unreal object by received STP package item token
        // TODO: Package will handle multiple item tokens, add token map to each packet and search through it
        UObject* findObject(const std::string& itemToken) const;

        // Get package name from STP item token string
        void getPackageNameFromToken(const std::string& token, std::string& packageName) const;

        // Get package from package name
        SceneProtocol::NetworkBridge::BridgeProtocol::IPackage* getPackageFromPackageName(FString packageName) const;

        UMaterialInterface* getUMaterialByName (
            const std::string &materialName,
#if ENGINE_MAJOR_VERSION >= 4 && ENGINE_MINOR_VERSION >= 23
            const std::map<FName, SceneProtocol::NetworkBridge::BridgeProtocol::IPackage*, cmpFNames>& packages,
#else
            const std::map<FName, SceneProtocol::NetworkBridge::BridgeProtocol::IPackage*>& packages,
#endif
            const std::map<SceneProtocol::NetworkBridge::BridgeProtocol::IPackage*, std::vector<UObject*>>& packageObjects
        );

        std::vector<UMaterialInterface*> getMaterials(const SceneTransmissionProtocol::Client::Response *response);
        UTexture2D* getTexture(const std::string& uniqueName) const;

      private:
        static PackageMapper*  _instance;

        // Removes a package the PackageManagers's internal caches and deletes the underlying package.
        void removePackageByName(FName& token);

        // Get Unreal objects by class names
        // This is used in map initialization to iterate through Unreal items
        void getObjectsByClassNames(const TArray<FName>& classNames, TArray<UObject*>& inMemoryObjects);

        // Generate map helper method
        template <typename T>
        void generateMapsHelper(const TArray<FName, FDefaultAllocator>& classNames, bool (PackageMapper::*RegFunc)(T*));

        //
        // Event handlers
        //

        // Actor deletion event removes an actor from the map
        void onActorDeletion(AActor* actor);

        // Asset deletion event removes an asset from the map
        void onAssetsPreDeleted(const TArray<UObject*>& objects);

        // Asset deletion event removes an asset from the map
        void onAssetsDeleted(const TArray<UClass*>& DeletedAssetClasses);

        // In memory asset deletion removes an asset from the map
        void onInMemoryAssetsDeleted(UObject *Object);

        // When the world is being added iterate over all items and generate packet maps
        void onWorldAdded(UWorld* world);

        // Remove level event clears the packet maps
        void onLevelRemoved(ULevel* level, UWorld* world);

        //
        // Register items to packet methods
        //

        bool registerMeshInstance(AStaticMeshActor* staticMeshActor);
        bool registerMesh(UStaticMesh* staticMesh);
        bool registerLight(UObject* lightObject);
        bool registerMaterialDefinition(UMaterial* material);
        bool registerMaterial(UMaterialInstanceConstant* matInstance);
        bool registerTexture(UTexture2D* texture);
        bool registerCamera(ACameraActor* cameraPawn);

        // Register items to packet helper methods
        bool checkUserDataValidity(USceneProtocolUserData* userData);
        void registerObjectVect(std::vector<UObject*>& objects, USceneProtocolUserData* userData, FString packageRoot, UPackage* outermostPackage);

        // STP unique token name mapping
        // maps STP unique item token string -> hashed package name
        typedef std::pair<std::string, FName>                                                      TokenToPackageNameMapVal;
        std::unordered_map<std::string, FName>                                                     _tokenToPackageNameMap;

        // Package maps, maps package names to package objects
        // We use FNames internally, as the comparison operation between them
        // is a lot cheaper than strings.
#if ENGINE_MAJOR_VERSION >= 4 && ENGINE_MINOR_VERSION >= 23
        std::map<FName, SceneProtocol::NetworkBridge::BridgeProtocol::IPackage*, cmpFNames>        _packageNameToPackageMap;
#else
        std::map<FName, SceneProtocol::NetworkBridge::BridgeProtocol::IPackage*>                   _packageNameToPackageMap;
#endif

        // Package to objects map
        std::map<SceneProtocol::NetworkBridge::BridgeProtocol::IPackage*, std::vector<UObject*> >  _packageToObjectsMap;

        // Store asset tokens that are pending deletion
        std::forward_list<FName>                                                                   _assetTokensPendingDeletion;
      };
    }
  }
}

#endif
