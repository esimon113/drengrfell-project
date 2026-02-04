# Documentation for Behavior Tree Programming

## Nodes

A node is represented as a JSON object.
Each object has a key named `kind`, which sets the type of the node for deserialization purposes.

### And / Sequence

Processes the children. Breaks and returns `BTState::Failure` on the first unsuccessful child.
Returns `BTState::Success` if every child succeeds.

- `kind`: `and` or `sequence`
- `children`: Array of node objects to be processed.

### Or / Selector

Processes the children. Breaks and returns `BTState::Success` on the first successful child.
Returns `BTState::Failure` if every child fails.

- `kind`: `or` or `selector`
- `children`: Array of node objects to be processed.

### Inverter

Processes `child` and inverts the `BTState` result.
`BTState::Success` if `child` fails, `BTState::Failure` if `child` succeeds.

- `kind`: `inverter`
- `child`: Node object to be processed.

### Succeeder

Processes `child` and returns `BTState::Success` regardless of failure.

- `kind`: `Succeeder`
- `child`: Node object to be processed.

### Until Failure Repeater

Repeats `child` until the node returns `BTState::Failure`.

- `kind`: `untilFailureRepeater`
- `child`: Node object to be processed.

### Repeater

Repeats `child` a number of `times`.

- `kind`: `repeater`
- `times`: Number of iterations.
- `child`: Node object to be processed.

### Function

Processes a command of the games command registry.

- `kind`: `function`
- `name`: Name of a command in the command registry.
- `args`: Object of arguments which are given to the command on processing.
          The arguments depend on the specific command.
          Unknown arguments are ignored.
  - The values of the `args`-object are evaluated on processing.
    Each `*VAR` is evaluated to the stored value of `VAR`, allowing easy retrieval of stored values.
  - `store`: The variable a command result is stored into. Defaults to `ans`.

### Shorthand notation

Usually, describing the full function node object in the JSON is redundant.
Therefore, a shorthand notation exists.

```json
{
	"getUniformInt": {
		"start": 0,
		"end": "*ans"
	}
}
```

is interpreted as

```json
{
	"kind": "function",
	"name": "getUniformInt",
	"args": {
		"start": 0,
		"end": "*ans"
	}
}
```

To use this notation, the object must not have a `kind` key.

## Command Registry

Register commands on the games startup (any constructor or init-method) like so

```cpp
aiSystem->getCommandRegistry().registerCommand(
	"commandName",
	[](const BTContext& context, const BTF::Args& args) {
		// do something with context and args
		return BTState::Success;
	}
);
```

**Don't forget that the behavior tree is ment for controlling multiple AI enemies.
To distinguish these enties, use `context.entity` to get the correct id.**

### Working with objects / this-pointer

If you are inside an object and need to access its internals,
you want to capture the this-pointer like so

```cpp
aiSystem->getCommandRegistry().registerCommand(
	"commandName",
	[this](const BTContext& context, const BTF::Args& args) {
		this->soos = 123;
	}
);
```

In this case you have to unregister the command on the objects destruction.
Otherwise, you travel into the unchartered realm of C++ undefined behavior,
because the this-pointer of the lambda becomes dangling when the object is destructed.
To unregister the command before destruction of the object, you just have to use

```cpp
aiSystem->getCommandRegistry().unregisterCommand("commandName");
```

in the objects destructor or deinit-method.

### Getting stored variable values

To get the JSON value of the argument `KEY`, defaulting to `DEFAULT` and getting it as a `TYPE`, use

```cpp
BTF::getArg<TYPE>(args, "KEY", DEFAULT)
```

### Storing command result

Cast `RESULT` into type `TYPE`. `TYPE` has to be any type listed under "BT Value Types".
Then store it into the variable specified by the `store`-key of the function `args`.

```cpp
BTF::store<TYPE>(context, args, RESULT);
```

## BTStates

- Success: Node was processed successfully.
- Failure: Node was processed unsuccessfully.
- Invalid: A problem was encountered the game could not handle. If this occurs, either the BTJson is erroneous or the games code.
- Running: Node is running. Maybe it succeeds or fails in the next process of the tree.

## BT Value Types

- `BTValueType`: any of the following
- Primitive
  - `BTString`: `std::string`
  - `BTNumber`: `double`
  - `BTBoolean`: `bool`
- Collections
  - `BTArray`: `std::vec<std::shared_ptr<BTValueType>>`
  - `BTObject`: `std::unordered_map<std::string, std::shared_ptr<BTValueType>>`
