cmake_minimum_required(VERSION 3.22)
project(learned_dht)

set(CMAKE_CXX_STANDARD 17)

add_executable(learned_dht main.cpp topologies/constdisttopology.C topologies/dvgraph.C topologies/e2easymgraph.C topologies/e2egraph.C topologies/e2elinkfailgraph.C topologies/e2etimegraph.C topologies/euclidean.C topologies/euclideangraph.C topologies/g2graph.C topologies/gtitm.C topologies/randomgraph.C topologies/topologyfactory.C protocols/accordion.C protocols/chord.C protocols/chordfinger.C protocols/chordfingerpns.C protocols/chordonehop.C protocols/chordtoe.C protocols/kademlia.C protocols/kelips.C protocols/koorde.C protocols/onehop.C protocols/protocolfactory.C protocols/ratecontrolqueue.C protocols/sillyprotocol.C protocols/tapestry.C p2psim/bighashmap.cc p2psim/bighashmap_arena.cc p2psim/condvar.C p2psim/event.C p2psim/eventgenerator.C p2psim/eventqueue.C p2psim/eventqueueobserver.C p2psim/network.C p2psim/node.C p2psim/observed.C p2psim/p2protocol.C p2psim/p2psim.C p2psim/packet.C p2psim/parse.C p2psim/rpchandle.C p2psim/threaded.C p2psim/threadmanager.C p2psim/tmgdmalloc.C p2psim/topology.C observers/chordobserver.C observers/datastoreobserver.C observers/kademliaobserver.C observers/kelipsobserver.C observers/observerfactory.C observers/onehopobserver.C observers/protocolobserver.C observers/tapestryobserver.C misc/datastore.C misc/simplex.c misc/vivaldinode.C misc/vivalditest.C libtask/channel.c libtask/context.c libtask/print.c libtask/task.c libtask/task.c libtask/tprimes.c failuremodels/constantfailuremodel.C failuremodels/failuremodelfactory.C failuremodels/roundtripsfailuremodel.C events/eventfactory.C events/netevent.C events/p2pevent.C events/simevent.C eventgenerators/churneventgenerator.C eventgenerators/churnfileeventgenerator.C eventgenerators/eventgeneratorfactory.C eventgenerators/fileeventgenerator.C eventgenerators/sillyeventgenerator.C libtask/asm.S libtask/asm.S
        protocols/learned_dht.C
        protocols/learned_dht.h
        learned_hash_function/rmi.cpp
        learned_hash_function/rmi.h
        learned_hash_function/rmi_data.h
        protocols/chordv.C
        protocols/chordv.h
        observers/learneddhtobserver.h
        observers/learneddhtobserver.C
        eventgenerators/vnodeeventgenerator.h
        eventgenerators/vnodeeventgenerator.C
        protocols/chord_overlay.C
        protocols/chord_overlay.h
        protocols/marques.C
        protocols/marques.h
        observers/marquesobserver.C
        eventgenerators/marqueseventgenerator.C
        eventgenerators/marqueseventgenerator.h
        topologies/PlanetLabData.C
        topologies/PlanetLabData.h
        topologies/PlanetLabData.C
        learned_hash_function/pgm/morton_nd.hpp
        learned_hash_function/pgm/pgm_index.hpp
        learned_hash_function/pgm/pgm_index_dynamic.hpp
        learned_hash_function/pgm/pgm_index_variants.hpp
        learned_hash_function/pgm/piecewise_linear_model.hpp
        learned_hash_function/pgm/sdsl.hpp
        learned_hash_function/pgm.cpp
        learned_hash_function/rs.cpp
        learned_hash_function/rs/builder.h
        learned_hash_function/rs/common.h
        learned_hash_function/rs/multi_map.h
        learned_hash_function/rs/radix_spline.h
        learned_hash_function/rs/serializer.h
)

# set (CMAKE_CXX_FLAGS   "${CMAKE_CXX_FLAGS} -fpermissive")

# Search OpenSSL
find_package(OpenSSL REQUIRED)
if (OPENSSL_FOUND)
    # Add the include directories for compiling
    target_include_directories(${PROJECT_NAME} PUBLIC ${OPENSSL_INCLUDE_DIR})

    # Add the static lib for linking
    target_link_libraries(${PROJECT_NAME} OpenSSL::SSL OpenSSL::Crypto)

    message(STATUS "Found OpenSSL ${OPENSSL_VERSION}")

else()

    message(STATUS "OpenSSL Not Found")

endif()