# EntityNetwork
A network agnostic library to managed entity synchronization in a client/server model for gaming with eventual rollback features.

## Design
EntityNetwork is a system to faclititate the synchronization of entities (game object) across a network. It is written in C++ 17 and attemps to use a many modern concepts as possible. It is written to be thread save and network agnostic. Enity Network does not contain any direct networking code, but reads and writes data packets via a push/pop message mechanism. This allows it to be used with any desired network trasnport library. Sample implmentations are provided using a single header fork of ENet. The deisgn calls for tracking of what entities and entity properties have been sent to all clients in order to implment delta states and other advanced features. The design will not implment compression or encryption, that is up to the transport mechanism.

The design is a work in progress so it may change over time. The eventual goal is to make the library be able to keep track of entity states and provide state rollback services for an authoritive server, similar to what is provided by the GGPO library https://www.ggpo.net/ , but using a client/server model rather than a peer to peer model.

## Concepts
EntityNetwork has few basic concepts it uses to provide a generic interface for library clients.

### Entity
An object that is tracked in the world. All Entities have properties and a world cluster location. As entities are created and destoryed the library will sync any data assocaited with an entity to all worlds (server and client). Entities are given a type and a type descriptior that defines the data associated with each entity to allow the library to only send changed data. Specific entities can be flaged as "avatar" entities so that the library can do spatial partitioning to only need updates of nearby entities.

### EnityController
A player. Entity controllers are the owners of entities created and stored in the world. Enity Controllers be remote players and/or server side AI objects. All Entiteis are associated with some form of entity controller.

### Property
Properties are data fields that are associated with entities, entity controlers, and the world itself. These fields contain data that will be synced to all linked worlds via message packages. 

### World
A world is a set of entity controllers and associated entities. There are two forms of worlds, Server and Client. Both types maintain a sycned state of entities and properties, but have different roles and permissions in the process.

### Remote Procedure Calls (RPC)
The server can define a set of named procedures with arguments and sync those defintions with all clients. These procedures fall into two categories, functions the client can call on the server, and funcitons the server can call on the client. The client and server have the option to register native funciton pointers that are called when these procesdures are triggered called by the other side of the network. The library will handle packging up the nessisary arguments and getting them synced.

All RPC definitions are provided to the client on connection (or when new items are registered), so the client system can cache or bind them to any native or local sciripting code that it needs to. During runtime all procedures and arguments are referenced by ID to save bandwith.

#### Examples
	Client -> Server procedure
		"RequestSpawn", No Arguments.
		Server installs a HandleSpawn funciton pointer that handles the request and is given the requesting client's entity controller.
	
	Server -> Single client procedure.
		"Direct Chat Message", Source ID, Message String arguments
		Client installs a HandleDirectChat function pointer that handles the display of chat from a single user. Server calls the proceedure with the target client's entity controller and provides the Source ID and Message String data as arguments.
		
	Server -> All clients procedure.
		"Play Sound", Sound ID, Sound Volume, Sound Postion Vector(3 floats)
		Client installs a HandlePlaySound function pointer that handles the playing of a sound. Server calls the proceedure and provides the sound info as arguments, and the library handles sending it to all clients.
		
# ToDo
* entity definitions
  * avatar
  * entity properties
* entity create and sync
* child entities
  * linked and unlinked
* spatial partitions
* near avatar updates
* update ticks
* last sent tick
* last ack tick
* entitiy history

