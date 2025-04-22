# Lab Work 8: Dataflow Adapters Library (C++)

This lab focuses on designing a **modular and composable data processing library** using STL-like adapters and lazy evaluation.  
The resulting syntax mimics functional programming pipelines and enables elegant data transformations on files, containers, and streams.

---

## 🎯 Objective

Create a set of C++ adapters to simplify operations over:

- Filesystem data (e.g., reading `.txt` files from directories)
- File content (e.g., parsing and tokenizing)
- In-memory containers
- Transformations, filtering, and aggregations

Adapters are chained using the pipe `|` operator, similar to functional data processing frameworks.

---

## ✅ Example Use Case

```cpp
Dir(argv[1], recursive) 
    | Filter([](std::filesystem::path& p){ return p.extension() == ".txt"; })
    | OpenFiles()
    | Split("\n ,.;")
    | Transform(
        [](std::string& token) { 
            std::transform(token.begin(), token.end(), token.begin(), [](char c){return std::tolower(c);});
            return token;
        })
    | AggregateByKey(
        0uz, 
        [](const std::string&, size_t& count) { ++count; },
        [](const std::string& token) { return token; }
      )
    | Transform([](const std::pair<std::string, size_t>& stat) {
        return std::format("{} - {}", stat.first, stat.second);
      })
    | Out(std::cout);
```

> This code finds word frequencies in all `.txt` files in a directory recursively.

---

## 🔧 Required Adapters

| Adapter        | Description |
|----------------|-------------|
| `Dir`          | Returns all files in a directory (recursively) |
| `OpenFiles`    | Opens file streams from paths |
| `Split`        | Splits input by delimiters |
| `Out`          | Outputs elements to a stream |
| `AsDataFlow`   | Converts a container to a data stream |
| `Transform`    | Applies a function to each element |
| `Filter`       | Filters elements based on a predicate |
| `Write`        | Outputs elements with a custom separator |
| `AsVector`     | Collects the stream into a `std::vector` |
| `Join`         | Performs a left join on two streams |
| `KV`           | Key-value pair structure |
| `JoinResult`   | Holds joined result of two `KV` streams |
| `DropNullopt`  | Filters out `std::nullopt` values from a stream of `std::optional<T>` |
| `SplitExpected`| Splits processing into success/error pipelines based on `expected` |
| `AggregateByKey` | Aggregates values by key (non-lazy) |

---

## ⚙️ Constraints

- All adapters except `AggregateByKey` and `Join` must use **constant memory**
- Lazy evaluation is expected wherever applicable
- Adapters should **not** own the data; they process it by reference
- `std::ranges` is **not allowed**, but inspired behavior is encouraged

---

## 🧪 Testing

- All adapters must be covered by **unit tests**
- Use [Google Test](https://google.github.io/googletest/) as your testing framework
- Provided tests are partial — you are expected to improve coverage
- Test coverage directly affects your grade

---

## 🛠 Implementation Notes

- Adapters must support `range-based for` loops  
  See: [Range-for Requirements](https://en.cppreference.com/w/cpp/language/range-for)

- Modular design is crucial: each adapter should be testable in isolation
- Avoid code duplication — use templates and concepts wisely
- Consider using views, proxies, or iterators internally

---

## 💡 Inspiration

Your pipeline is similar to `std::ranges` or LINQ in C#. While `std::ranges` is **not allowed**, understanding it can help you design composable, lazy systems.

Try rewriting your **Lab Work 1** using the adapter framework you build in this lab — it’s a great way to test design quality.

---

## 🧠 Key Topics

- Lazy evaluation  
- Template metaprogramming  
- Iterators and proxies  
- Custom STL-style interfaces  
- Functional-style programming in C++


---

## 👨‍💻 Author

1st-year CS student @ ITMO University  
GitHub: [npapaHAHA](https://github.com/npapaHAHA)


