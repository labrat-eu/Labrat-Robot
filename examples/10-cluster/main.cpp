#include <labrat/lbot/manager.hpp>
#include <labrat/lbot/node.hpp>
#include <labrat/lbot/cluster.hpp>

#include <string>

// Cluster
//
// You can use node cluster to logically group your nodes together.
// This can be useful when developing a package that uses multiple nodes but you don't want the user to start them up separately.
//
// This example showcases how to work with node cluster.

class ExampleNode : public lbot::Node {
public:
  ExampleNode(const lbot::NodeEnvironment &environment) : lbot::Node(environment) {
    getLogger().logInfo() << "Example node has been started.";
  }

  ~ExampleNode() {
    getLogger().logInfo() << "Node is shutting down.";
  }
};

// Clusters must inherit from lbot::Cluster.
class ExampleCluster : public lbot::Cluster {
public:
  ExampleCluster(const lbot::ClusterEnvironment environment) : lbot::Cluster(environment) {
    // Construct nodes in the cluster constructor.
    addNode<ExampleNode>("node_a");
    addNode<ExampleNode>("node_b");
    addNode<ExampleNode>("node_c");
  }
};

int main(int argc, char **argv) {
  lbot::Manager::Ptr manager = lbot::Manager::get();

  // We can now create all nodes contained by the cluster in one call.
  manager->addCluster<ExampleCluster>("cluster_a");

  // Remove the cluster.
  manager->removeCluster("cluster_a");

  return 0;
}
