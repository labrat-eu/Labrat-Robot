#include <labrat/lbot/manager.hpp>

#include <thread>

#include <gtest/gtest.h>

#include <helper.hpp>

inline namespace labrat {
namespace lbot::test {

class SemanticsTest : public LbotTest {};

TEST_F(SemanticsTest, DISABLED_sender) {
  labrat::lbot::Manager::Ptr manager = labrat::lbot::Manager::get();

  std::shared_ptr<TestNode> node(manager->addNode<TestNode>("node"));

  bool called = false;

  Node::Sender<TestFlatbuffer>::Ptr sender_a = node->addSender<TestFlatbuffer>("topic_a");
  Node::Sender<TestMessage>::Ptr sender_b = node->addSender<TestMessage>("topic_b");
  Node::Sender<TestMessageConv>::Ptr sender_c = node->addSender<TestMessageConv>("topic_c");
  Node::Sender<TestMessageConvPtr>::Ptr sender_d = node->addSender<TestMessageConvPtr>("topic_d", &called);

  TestMessage message_raw;
  sender_a->put(message_raw);
  sender_b->put(message_raw);

  TestContainer message_container;
  sender_c->put(message_container);
  sender_d->put(message_container);
}

static void callbackFunc(const TestMessage &, int *) {}
static void callbackFuncNoPtr(const TestMessage &) {}
static void callbackFuncConv(const TestContainer &, int *) {}
static void callbackFuncConvNoPtr(const TestContainer &) {}

TEST_F(SemanticsTest, DISABLED_receiver) {
  labrat::lbot::Manager::Ptr manager = labrat::lbot::Manager::get();

  std::shared_ptr<TestNode> node(manager->addNode<TestNode>("node"));

  bool called = false;

  Node::Receiver<TestFlatbuffer>::Ptr receiver_a = node->addReceiver<TestFlatbuffer>("topic_a");
  Node::Receiver<TestMessage>::Ptr receiver_b = node->addReceiver<TestMessage>("topic_b");
  Node::Receiver<TestMessageConv>::Ptr receiver_c = node->addReceiver<TestMessageConv>("topic_c");
  Node::Receiver<TestMessageConvPtr>::Ptr receiver_d = node->addReceiver<TestMessageConvPtr>("topic_d", &called);

  TestMessage message_raw;
  message_raw = receiver_a->latest();
  message_raw = receiver_b->latest();

  TestContainer message_container;
  message_container = receiver_c->latest();
  message_container = receiver_d->latest();

  int test;
  receiver_a->setCallback(&callbackFunc, &test);
  receiver_b->setCallback(&callbackFuncNoPtr);
  receiver_c->setCallback(&callbackFuncConv, &test);
  receiver_d->setCallback(&callbackFuncConvNoPtr);
}

static TestMessage handlerFunc(const TestMessage &, int *) {return{};}
static TestMessage handlerFuncNoPtr(const TestMessage &) {return{};}
static TestContainer handlerFuncConv(const TestContainer &, int *) {return{};}
static TestContainer handlerFuncConvNoPtr(const TestContainer &) {return{};}

TEST_F(SemanticsTest, DISABLED_server) {
  labrat::lbot::Manager::Ptr manager = labrat::lbot::Manager::get();

  std::shared_ptr<TestNode> node(manager->addNode<TestNode>("node"));

  Node::Server<TestFlatbuffer, TestFlatbuffer>::Ptr server_a = node->addServer<TestFlatbuffer, TestFlatbuffer>("service_a");
  Node::Server<TestFlatbuffer, TestMessage>::Ptr server_b = node->addServer<TestFlatbuffer, TestMessage>("service_b");
  Node::Server<TestMessage, TestMessage>::Ptr server_c = node->addServer<TestMessage, TestMessage>("service_c");
  Node::Server<TestMessageConv, TestMessageConv>::Ptr server_d = node->addServer<TestMessageConv, TestMessageConv>("service_d");
  Node::Server<TestMessageConvPtr, TestMessageConv>::Ptr server_e = node->addServer<TestMessageConvPtr, TestMessageConv>("service_e");
  Node::Server<TestMessageConvPtr, TestMessageConvPtr>::Ptr server_f = node->addServer<TestMessageConvPtr, TestMessageConvPtr>("service_f");

  int test;
  server_a->setHandler(handlerFunc, &test);
  server_b->setHandler(handlerFunc, &test);
  server_c->setHandler(handlerFuncNoPtr);
  server_d->setHandler(handlerFuncConv, &test);
  server_e->setHandler(handlerFuncConv, &test);
  server_f->setHandler(handlerFuncConvNoPtr);
}

TEST_F(SemanticsTest, DISABLED_client) {
  labrat::lbot::Manager::Ptr manager = labrat::lbot::Manager::get();

  std::shared_ptr<TestNode> node(manager->addNode<TestNode>("node"));

  Node::Client<TestFlatbuffer, TestFlatbuffer>::Ptr client_a = node->addClient<TestFlatbuffer, TestFlatbuffer>("service_a");
  Node::Client<TestFlatbuffer, TestMessage>::Ptr client_b = node->addClient<TestFlatbuffer, TestMessage>("service_b");
  Node::Client<TestMessage, TestMessage>::Ptr client_c = node->addClient<TestMessage, TestMessage>("service_c");
  Node::Client<TestMessageConv, TestMessageConv>::Ptr client_d = node->addClient<TestMessageConv, TestMessageConv>("service_d");
  Node::Client<TestMessageConvPtr, TestMessageConv>::Ptr client_e = node->addClient<TestMessageConvPtr, TestMessageConv>("service_e");
  Node::Client<TestMessageConvPtr, TestMessageConvPtr>::Ptr client_f = node->addClient<TestMessageConvPtr, TestMessageConvPtr>("service_f");

  {
    TestMessage request;
    TestMessage response;
    response = client_a->callSync(request);
    response = client_b->callSync(request);
    response = client_c->callSync(request);
  }

  {
    TestContainer request;
    TestContainer response;
    response = client_d->callSync(request);
    response = client_e->callSync(request);
    response = client_f->callSync(request);
  }

  {
    TestMessage request;
    std::shared_future<TestMessage> response;
    response = client_a->callAsync(request);
    response = client_b->callAsync(request);
    response = client_c->callAsync(request);
  }

  {
    TestContainer request;
    std::shared_future<TestContainer> response;
    response = client_d->callAsync(request);
    response = client_e->callAsync(request);
    response = client_f->callAsync(request);
  }
}

}  // namespace lbot::test
}  // namespace labrat
