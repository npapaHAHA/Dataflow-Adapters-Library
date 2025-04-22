#ifndef PROCESSING_H
#define PROCESSING_H

#include <optional>
#include <unordered_map>
#include <vector>
#include <filesystem>
#include <string>
#include <system_error>
#include <fstream>
#include <sstream>
#include <iostream>
#include <unordered_set>
#include <type_traits>
#include <iterator>
#include <utility>
#include <algorithm>
#include <cctype>
#include <exception>
#include <expected>

template <typename T>
auto to_std_string(const T &t) -> decltype(t.str()) { return t.str(); }
inline std::string to_std_string(const std::string &s) { return s; }

template<typename Key, typename Value>
struct KV {
    Key key;
    Value value;
};
template<typename Base, typename Joined>
struct JoinResult {
    Base base;
    std::optional<Joined> joined;
    bool operator==(const JoinResult &other) const {
        return base == other.base && joined == other.joined;
    }
};

struct Dir {
    std::filesystem::path path;
    bool recursive;
    Dir(const char* p, bool rec) : path(p), recursive(rec) {}
    std::vector<std::filesystem::path> operator()() const {
        std::vector<std::filesystem::path> files;
        if (recursive) {
            for (const auto &entry : std::filesystem::recursive_directory_iterator(path)) {
                if (entry.is_regular_file()) {
                    files.push_back(entry.path());
                }
            }
        } else {
            for (const auto &entry : std::filesystem::directory_iterator(path)) {
                if (entry.is_regular_file()) {
                    files.push_back(entry.path());
                }
            }
        }
        return files;
    }
};
template<typename Adapter>
auto operator|(const Dir &d, Adapter adapter) {
    auto files = d();
    return adapter(std::move(files));
}


// struct OpenFiles {
//     template<typename Range>
//     auto operator()(Range &&paths) const {
//         std::vector<std::string> contents;
//         for (const auto &path : paths) {
//             std::ifstream file(path);
//             if (!file.is_open()) {
//                 continue;
//             }
//             std::ostringstream buffer;
//             buffer << file.rdbuf();
//             contents.push_back(buffer.str());
//         }
//         return contents;
//     }
// };
//
// template<typename Range>
// auto operator|(Range &&r, const OpenFiles &op) {
//     return op(std::forward<Range>(r));
// }

// template<typename Range>
// auto operator|(Range &&paths, const OpenFiles &open_files) {
//     return open_files(std::forward<Range>(paths));
// }

struct Split {
    std::string delimiters;
    std::unordered_set<char> delimiterSet;
    explicit Split(const std::string &delims)
        : delimiters(delims), delimiterSet(delims.begin(), delims.end()) {}
    template<typename Range>
    auto operator()(Range &&input) const {
        std::vector<std::string> tokens;
        for (const auto &element : input) {
            std::string line = to_std_string(element);

            std::string token;
            for (char c : line) {
                if (delimiterSet.count(c)) {
                    tokens.push_back(token);
                    token.clear();
                } else {
                    token.push_back(c);
                }
            }
            tokens.push_back(token);
        }
        return tokens;
    }
};
template<typename Range>
auto operator|(Range &&input, const Split &splitter) {
    return splitter(std::forward<Range>(input));
}


struct Out {
    std::ostream &os;
    explicit Out(std::ostream &out = std::cout) : os(out) {}
    template<typename Range>
    void operator()(const Range &data) const {
        for (const auto &item : data) {
            os << item << '\n';
        }
    }
};
template<typename Range>
void operator|(const Range &data, const Out &out) {
    out(data);
}

template<typename Container> class AsDataFlow;
template<typename T> struct is_AsDataFlow : std::false_type {};
template<typename X> struct is_AsDataFlow<AsDataFlow<X>> : std::true_type {};

template<typename Container>
class AsDataFlow {
public:
    using value_type = typename Container::value_type;
    Container data_;
    template <typename C,
              typename = std::enable_if_t<
                  std::is_same_v<typename std::decay_t<C>::value_type, value_type>
              >
    >
    explicit AsDataFlow(C&& original) {

        if constexpr (is_AsDataFlow<std::decay_t<C>>::value) {
            auto &ref = original;
            data_ = std::move(ref.data_);
        } else {
            auto &ref = original;
            data_.reserve(ref.size());
            for (auto &elem : ref) {
                data_.push_back(std::move_if_noexcept(elem));
            }
        }
    }
    auto begin()       { return data_.begin(); }
    auto end()         { return data_.end(); }
    auto begin() const { return data_.begin(); }
    auto end()   const { return data_.end(); }

    template<typename Adapter>
    auto operator|(Adapter adapter) {
        return adapter(std::move(data_));
    }
};
template <typename C>
AsDataFlow(C&&) -> AsDataFlow<std::vector<typename std::decay_t<C>::value_type>>;


template<typename Func>
struct Transform {
    Func func;
    explicit Transform(Func f) : func(std::move(f)) {}
    template<typename Range>
    auto operator()(Range &&input) const {
        using InputType  = std::remove_reference_t<decltype(*std::begin(input))>;
        using ResultType = decltype(func(std::declval<InputType&>()));
        std::vector<ResultType> out;
        auto first = input.begin();
        auto last  = input.end();
        out.reserve(std::distance(first, last));
        for (auto it = first; it != last; ++it) {
            out.push_back(func(*it));
        }
        return out;
    }
};
template<typename Range, typename Func>
auto operator|(Range &&input, const Transform<Func> &t) {
    return t(std::forward<Range>(input));
}

struct Write {
    std::ostream &os;
    std::string delimiter;
    Write(std::ostream &out, const std::string &delim)
        : os(out), delimiter(delim) {}
    Write(std::ostream &out, char delim)
        : os(out), delimiter(1, delim) {}
    template<typename Range>
    Range operator()(Range &&data) const {
        for (auto &item : data) {
            os << item << delimiter;
        }
        return std::forward<Range>(data);
    }
};
template<typename Range>
auto operator|(Range &&data, const Write &writer) {
    return writer(std::forward<Range>(data));
}

struct AsVector {
    template<typename Range>
    auto operator()(Range &&range) const {
        using Value = typename std::decay_t<Range>::value_type;
        return std::vector<Value>(range.begin(), range.end());
    }
};
template<typename Range>
auto operator|(Range &&range, AsVector a) {
    return a(std::forward<Range>(range));
}

struct DefaultKeyExtractor {
    template<typename T>
    auto operator()(const T &x) const -> decltype(x.key) {
        return x.key;
    }
};
template<typename T, typename = void>
struct ValueExtractor {
    T operator()(const T &x) const { return x; }
};
template<typename K, typename V>
struct ValueExtractor<KV<K,V>> {
    V operator()(const KV<K,V> &kv) const { return kv.value; }
};

template<typename RightRange,
         typename KeyExtractorBase   = DefaultKeyExtractor,
         typename KeyExtractorJoined = DefaultKeyExtractor>
class Join {
public:
    template<typename R>
    Join(R&& right,
         KeyExtractorBase key_base     = {},
         KeyExtractorJoined key_joined = {})
        : right_(std::forward<R>(right))
        , key_base_(std::move(key_base))
        , key_joined_(std::move(key_joined))
    { }
    template<typename LeftRange>
    auto operator()(LeftRange &&left) const {
        using RightElem = typename std::decay_t<RightRange>::value_type;
        std::unordered_map<decltype(key_joined_(std::declval<RightElem>())),
                           RightElem> right_map;
        for (auto &elem : right_) {
            right_map.emplace(key_joined_(elem), elem);
        }
        ValueExtractor<typename std::decay_t<LeftRange>::value_type> baseValX;
        ValueExtractor<RightElem> joinedValX;
        using BaseType   = decltype(baseValX(*std::begin(left)));
        using JoinedType = decltype(joinedValX(*std::begin(right_)));
        std::vector<JoinResult<BaseType, JoinedType>> out;
        auto first = left.begin();
        auto last  = left.end();
        out.reserve(std::distance(first, last));
        for (auto &base_elem : left) {
            auto key     = key_base_(base_elem);
            auto baseVal = baseValX(base_elem);
            auto it      = right_map.find(key);
            if (it != right_map.end()) {
                auto joinedVal = joinedValX(it->second);
                out.push_back({ baseVal, joinedVal });
            } else {
                out.push_back({ baseVal, std::nullopt });
            }
        }
        return out;
    }
private:
    std::decay_t<RightRange> right_;
    KeyExtractorBase key_base_;
    KeyExtractorJoined key_joined_;
};
template<typename R>
Join(R&&) -> Join<std::decay_t<R>, DefaultKeyExtractor, DefaultKeyExtractor>;
template<typename R, typename B, typename J>
Join(R&&, B, J) -> Join<std::decay_t<R>, B, J>;
template<typename L, typename R, typename B, typename J>
auto operator|(L &&left, const Join<R,B,J> &joinObj) {
    return joinObj(std::forward<L>(left));
}

struct DropNullopt {
    template<typename Range>
    auto operator()(Range &&input) const {
        using Opt = typename std::decay_t<Range>::value_type;
        using T   = typename Opt::value_type;
        std::vector<T> out;
        auto first = input.begin();
        auto last  = input.end();
        out.reserve(std::distance(first, last));
        for (auto it = first; it != last; ++it) {
            if (it->has_value()) {
                out.push_back(it->value());
            }
        }
        return out;
    }
};
template<typename Range>
auto operator|(Range &&input, const DropNullopt &d) {
    return d(std::forward<Range>(input));
}

template<typename Predicate>
class Filter {
public:
    explicit Filter(Predicate p) : predicate_(std::move(p)) {}
    template<typename Range>
    auto operator()(Range &&input) const {
        using T = typename std::decay_t<Range>::value_type;
        std::vector<T> out;
        for (auto &elem : input) {
            if (predicate_(elem)) {
                out.push_back(elem);
            }
        }
        return out;
    }
private:
    Predicate predicate_;
};
// template<typename Predicate>
// Filter<Predicate> MakeFilter(Predicate p) {
//     return Filter<Predicate>(std::move(p));
// }
template<typename Range, typename Predicate,
         typename = std::void_t<typename std::decay_t<Range>::value_type>>
auto operator|(Range &&r, const Filter<Predicate> &f) {
    return f(std::forward<Range>(r));
}

struct OpenFiles {
    template<typename Range>
    auto operator()(Range &&paths) const {
        std::vector<std::string> contents;
        for (const auto &path : paths) {
            std::ifstream file(path);
            if (!file.is_open()) {
                continue;
            }
            std::ostringstream buffer;
            buffer << file.rdbuf();
            contents.push_back(buffer.str());
        }
        return contents;
    }
};

template<typename Range>
auto operator|(Range &&paths, const OpenFiles &op) {
    return op(std::forward<Range>(paths));
}

// В processing.h добавьте структуру AggregateByKeyAdapter и operator|
template <typename Init, typename Aggregator, typename KeyExtractor>
struct AggregateByKeyAdapter {
    Init init;
    Aggregator aggregator;
    KeyExtractor keyExtractor;

    template <typename Range>
    auto operator()(Range&& range) const {
        using Elem = typename std::decay_t<Range>::value_type;
        auto effKeyExtractor = [&](const Elem &x) -> decltype(auto) {
            if constexpr (std::is_invocable_v<KeyExtractor, const Elem&>)
                return keyExtractor(x);
            else
                return x;
        };
        using KeyT = std::decay_t<decltype(effKeyExtractor(*std::begin(range)))>;
        std::unordered_map<KeyT, Init> map;
        std::vector<KeyT> keyOrder;
        for (auto &element : range) {
            auto key = effKeyExtractor(element);
            if (!map.contains(key)) {
                keyOrder.push_back(key);
                map[key] = init;
            }
            if constexpr (std::is_invocable_v<Aggregator, decltype(map[key])&, decltype(element)>) {
                aggregator(map[key], element);
            } else if constexpr (std::is_invocable_v<Aggregator, decltype(element), decltype(map[key])&>) {
                aggregator(element, map[key]);
            }
        }
        std::vector<std::pair<KeyT, Init>> results;
        results.reserve(keyOrder.size());
        for (auto &k : keyOrder)
            results.emplace_back(k, map[k]);
        return results;
    }
};

template <typename Init, typename Aggregator, typename KeyExtractor>
auto AggregateByKey(Init init, Aggregator aggregator, KeyExtractor keyExtractor) {
    return AggregateByKeyAdapter<Init, Aggregator, KeyExtractor>{init, aggregator, keyExtractor};
}

template<typename Range, typename Init, typename Aggregator, typename KeyExtractor>
auto operator|(Range&& range, const AggregateByKeyAdapter<Init, Aggregator, KeyExtractor>& agg) {
    return agg(std::forward<Range>(range));
}

template<class T> struct is_std_expected : std::false_type {};
template<class V, class E>
struct is_std_expected<std::expected<V,E>> : std::true_type {};
template<typename C, typename = void>
struct is_expected_container : std::false_type {};
template<typename C>
struct is_expected_container<C, std::void_t<typename C::value_type>> {
private:
    using maybeExpected = typename C::value_type;
public:
    static constexpr bool value = is_std_expected<maybeExpected>::value;
};
template<typename T, typename E>
using SplitResult = std::pair<std::vector<E>, std::vector<T>>;
struct SplitExpected {
    template<typename U>
    SplitExpected(U) {}
    template<typename Container>
    auto operator()(Container &&c) const {
        using EType     = typename std::decay_t<Container>::value_type;
        using ValueType = typename EType::value_type;
        using ErrorType = typename EType::error_type;
        std::pair<std::vector<ErrorType>, std::vector<ValueType>> result;
        for (auto &&item : c) {
            if (item.has_value()) {
                result.second.push_back(item.value());
            } else {
                result.first.push_back(item.error());
            }
        }
        return result;
    }
};
template<typename Container,
         std::enable_if_t<is_expected_container<std::decay_t<Container>>::value, int> = 0>
auto operator|(Container &&c, const SplitExpected &splitter)
{
    return splitter(std::forward<Container>(c));
}

#endif // PROCESSING_H
