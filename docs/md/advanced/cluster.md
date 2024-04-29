@page cluster Node Cluster

# Basics
You can use node cluster to logically group your nodes together. This can be useful when developing a package that uses multiple nodes but you don't want the user to start them up separately.

In order to create a cluster, you have to define a class that is derived from `lbot::Cluster`.
```cpp
class ExampleCluster : public lbot::Cluster { ... }
```
You need to define at least one constructor for your cluster. The first argument of the constructor must be of the type `const lbot::ClusterEnvironment` and it must be forwarded to the parent constructor.
```cpp
ExampleCluster(const lbot::ClusterEnvironment environment, ...) : lbot::Cluster(environment) {}
```

# Manager
Cluster should never be created by directly calling their constructor. Instead you should use the central node manager. A call to `addCluster()` will construct the cluster and will take of any cleanup after your program has finished. The first argument of the `addCluster()` function is the name of the cluster. Afterwards custom arguments can be provided. You cannot add multiple cluster of the same type or name.
```cpp
manager->addCluster<ExampleCluster>("cluster_a", ...);
```
You can remove cluster from the manager explicitly. However you do not need to do this at the end of your program, as any remaining nodes will be removed automatically.
```cpp
manager->removeCluster("cluster_a");
```

# Inside the cluster
Inside of the cluster you can create the nodes contained by the cluster. This can be done through the `Cluster::addNode()` method. This function behaves similarly to the `Manager::addNode()` method. Typically the child nodes are created within the constructor of the cluster.
```cpp
addNode<ExampleNode>("node_a");
```