@page filter Filter

# Basics
Filter can be used to control which topics may be read by a trace plugin. This might be useful to prevent topics with very large messages (such as raw images/videos etc.) from causing performance issues.

There are two types of filters, blacklist and whitelist filters. A blacklist filter will allow a plugin to access all topics except the ones explicitly specified. A whitelist filter will block all topics by default and will only access on the specified topics.

# Usage
In order to create a filter you just need to declare a [lbot::Filter](@ref labrat::lbot::Filter) object. This will usually be done in you `main()` function as you will likely configure your plugins there as well.
```cpp
lbot::Filter filter;
```
In order to blacklist a topic you need to call the [Filter::blacklist()](@ref labrat::lbot::Filter::blacklist()) function and specify the name of the topic you want to blacklist. You can add as many topics to the blacklist as you want.
```cpp
filter.blacklist("/should/be/blocked");
```

In order to whitelist a topic you need to call the [Filter::whitelist()](@ref labrat::lbot::Filter::whitelist()) function and specify the name of the topic you want to whitelist. You can add as many topics to the whitelist as you want.
```cpp
filter.whitelist("/should/be/accepted");
```

@attention
You cannot blacklist and whitelist topics on the same filter. **If you call [Filter::whitelist()](@ref labrat::lbot::Filter::whitelist()) after [Filter::blacklist()](@ref labrat::lbot::Filter::blacklist()) on the same filter, the blacklist will be cleared and vice versa.**

Filters must be passed individually onto every plugin that should be affected by the filter.
```cpp
manager->addPlugin<ExamplePlugin>("example_plugin", filter, ...);
```