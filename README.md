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
Properties are data fields that are associated with entities and entity controolers. These fields contain data that will be synced to all linked worlds via message packages. 

### World
A world is a set of entity controllers and associated entities. There are two forms of worlds, Server and Client. Both types maintain a sycned state of entities and properties, but have different roles and permissions in the process.


