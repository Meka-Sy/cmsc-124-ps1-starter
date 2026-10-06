# Joint Analysis

> Pick 3 of the 10 categories. For each, pick a language that gives it to you for free and say what that language pays for it. "Python has dictionaries" isn't an answer. What does Python's dictionary cost in memory or in speed compared to what you built, and where would you notice?

## `dt_list.c` → List: Rust `Rc<Node>`

The cost is that `dt_list_free` can only free one cell because you have to be certain that no one else shares it, whereas `dt_list_cons` shares the tail by copying a pointer. This is where `Option<Rc<Node>>` comes in, fixing that by requiring each cell to track how many lists point at it. This way, the program can safely free a cell without requiring you to know who else is using it.

### What Rust pays

**Memory.** Each allocation carries a strong and weak count, 16 bytes on 64-bit.Current cell is value plus a pointer which makes the bookkeeping large.

**Speed.** Every time an `Rc` is cloned or dropped, it updates the reference count. However, simply traversing the list through references does not modify the count, so walking through the list adds no extra overhead. `Rc` is designed for single-threaded use, while `Arc` is thread-safe but relies on atomic operations, which are noticeably more expensive.

**Cycles leak.** Two cells that point at each other never reach zero. You'd need `Weak`.

**Observation.** When dropping a very large list, such as one containing 1 million cells, the default destructor recursively follows the tail, which can cause a stack overflow. To avoid this, you need to implement `Drop` iteratively. The implementation traverses the chain similarly to `dt_list_len`, but adds an extra check using `Rc::try_unwrap` at each step. It stops as soon as it encounters a tail that is still owned by another reference, ensuring that shared cells are never freed prematurely.

---

## `dt_map.c` → Dictionary: Python `dict`

Python will provide hashing, resizing, and collision handling in `{}`. It is also insertion-ordered by design, an entries array plus a sparse index table, which is close to order array but without the separate bookkeeping.

### What Python pays

**Memory.** Nothing is a raw value. Every key and value is a heap object with a ref count and type pointer, and an `int` is about 28 bytes compared to the current `dt_value`. Each slot also stores a cached hash. An empty dict is already around 64 bytes, so a million small dicts, one per record, adds up fast.

**Speed.** Each lookup goes through interpreter dispatch, a `hash()` call, cached for strings, and an equality check. Which makes it more slower than the one in C.

**Advantage.** It resized. The `bucket_count` is fixed at 5, so chains grow as n/5 and lookups degrade to O(n) on big maps. The current `dt_map_remove` also shifts arrays, and rehashes everything, which is O(n) per delete. While Python deletes in O(1) by leaving a tombstone, and compacting at the next resize.

**Observation.** Resize spikes on a single insert, and `RuntimeError` if you mutate a dict while iterating it. Keys must be hashable, and the dict holds a reference to the key where you copy it.

---

## `dt_ref.c` → Reference: Java (garbage collection)

It makes dangling references, and double frees impossible. There is no free, and an object lives as long as something reaches it. The whole `dt_ref` has no equivalent because the failure can not happen.

### What Java pays

**Memory.** The heap needs headroom, often 1.5 to 2 times the live data, for the collector to work efficiently. Every object has a header of roughly 12 to 16 bytes.

**Speed.** GC pauses, though modern collectors keep them short. Release timing is nondeterministic, so there is no equivalent to “freed exactly here.”

**Leaks.** A forgotten reference in a static map keeps its object alive forever. The `DT_ERR_LEAK` case would still exists, as “reachable but unused.”

**Observation.** In the occasional pauses, and in containers with packed memory limits, where the headroom matters.

---

> You wrote the tag check in `dt_value_as_int` by hand. Some languages don't let you. They make the tagged union a language construct, so the compiler writes the check for you, refuses to compile a read that skips it, and refuses to compile a set of cases that misses one. Rust's `enum` and `match` work this way, and so do ML's datatypes and Swift's enumerations with associated values. What does the C version let you do that a compiler enforcing the check wouldn't, and is any of it worth wanting?

In this activity, the tagged union is defined in `dt.h`. A `dt_value` contains a tag that identifies the kind of value stored, such as `DT_INT`, `DT_STR`, or `DT_MAP`, while the `as` union contains the corresponding data. Upon implementing `dt_value_as_int()`, we had to write the tag check ourselves before accessing `v.as.integer`:


C’s union allows the programmer to access union members without the compiler automatically enforcing a tag check. As a result, the C version gives the programmer maximum flexibility. The programmer can decide:

- **When to check the tag:** The programmer decides when to check the tag and how to handle mismatches.
- **Which cases to handle:** The compiler does not force every possible `dt_tag` variant to be handled in a `switch` or `if` statement.
- **When checking can be avoided:** If the programmer already knows that a value has a particular type from the surrounding program logic, they can choose not to perform another tag check.

However, this flexibility comes at the cost of safety. The programmer is responsible for maintaining the connection between the tag and the union member. If we forgot the `v.tag` check and accessed `v.as.integer` when the value was a string, the compiler would not automatically stop us. This can lead to incorrect behavior.

In contrast, modern languages like Rust use language features such as `enum` and `match` to enforce safer handling of different variants. In these languages, the compiler:

- Enforces safe access to the correct variant.
- Refuses to compile a `match` that does not handle all possible variants.
- Performs exhaustiveness checks to ensure every possible case is explicitly handled.

For this activity, we think that the compiler-enforced approach is more valuable. The `dt_value` has many possible alternatives, so manually checking the tag creates opportunities for mistakes. A language such as Rust can use its `enum` and `match` constructs to make the compiler enforce which variant is being handled and help detect missing cases. This gives up some of C’s freedom, but provides more safety. Therefore, C’s freedom is useful when direct control over representation and behavior is important.

---

> Your `dt_map` keeps insertion order separately from the hash buckets, which is memory spent on something no lookup uses. Argue the other side: describe a design that drops it, say what breaks, and say whether you'd ship it.

Our `dt_map` keeps insertion order separately using the `order` array. This uses additional memory even though the hash buckets do not need insertion order to perform key lookups.

An alternative design would be to remove the `order` and store only the keys, values, links, and hash buckets. The map would still be able to find a key by hashing it to a bucket and following the entries linked through `next`. However, operations that depend on insertion order would break or become more difficult. For example, iterating through the map in the same order that keys were inserted would no longer be directly supported. A function that returns keys by insertion position would also need to use another ordering method or accept that the order is not guaranteed.

We think removing `order` would be reasonable if the map only needs fast key lookup and does not promise insertion order. It would save memory and make the representation simpler. However, for our implementation, we would keep the `order` array because the assignment provides functions that work with insertion position. Therefore, we would not remove it for this implementation.

---

> Compare access after release with an allocation that remains unreleased at the driver's final check. What damage can each cause in a long-running server? How does that answer change for a command-line tool that exits in a second?

### USE-AFTER-FREE

A use-after-free occurs when a program continues to read or write though a pointer after the memory it points to has been deallocated. The pointer still holds the old address, but the program no longer owns that memory. The allocator may hand it to a different object, thus the program might read or overwrite data that belongs to something else. Such result would be an undefined behavior.

**Long-running server.** The damage would be severe as it could silently corrupt the data shared across programs, devices or user applications. It could be undetected for a long time, and such damage might worsen.

**Short command-line tool.** The result observed in the long-running server would still be observed in the short command-line took, as the damage would start when the pointer is used, the running time would not be taken into account. However, in a short run, corruption is given less time to spread.

### UNRELEASED ALLOCATION

A memory leak occurs when a program no longer needs a block of memory but never frees it, mostly because the pointer is lost or overwritten, which makes the block unreachable. A leak would not change the programs’s logic and results however it would consume memory and the consumption would accumulate. Memory leaks can be checked using leak checker which would reports the still unreleased allocation.

**Long-running server.** The memory used grows steadily with each leaking operation, then eventually it would cause slowdowns from failed allocations, or the process being killed for running out of memory.

**Short command-line tool.** The operating system reclaims all of the process’s memory at exit, which makes the leak harmless, and unnoticed, usually. However, when the program is re-used in a long-running program, then it would become risky.
