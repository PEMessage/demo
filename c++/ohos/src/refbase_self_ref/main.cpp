/*
 * Demo: automatic registration destroys an object inside its constructor.
 *
 * Build and run:
 *   ./run src/refbase_self_ref
 *
 * Catalog owns all Item objects, and every Item registers itself from its
 * constructor. Catalog::Register() takes a raw pointer and creates a local
 * strong reference. On success the map stores a second reference.
 *
 * On failure (duplicate id) that local reference is the only one, so returning
 * from Register() destroys the Item while its constructor is still running.
 * The caller still gets a non-null sptr<Item>, but it points to freed memory.
 */

#include "refbase.h"

#include <cstdio>
#include <cstring>
#include <map>
#include <sys/wait.h>
#include <unistd.h>

using namespace OHOS;

namespace {

class Item;

class Catalog {
public:
    static Catalog &Get();

    // Raw pointer on purpose: the implicit sptr conversion inside this method
    // is what makes a failed registration destroy the object being constructed.
    bool Register(int id, Item *item);

    void Clear();
    size_t Size() const { return items_.size(); }

private:
    std::map<int, sptr<Item>> items_;
};

int g_itemLive = 0;
bool g_itemDestroyedInCtor = false;

class Item : public RefBase {
public:
    explicit Item(int id) : id_(id), constructed_(false)
    {
        ++g_itemLive;
        printf("[Item ctor] begin   this=%p id=%d\n", static_cast<void *>(this), id_);

        if (!Catalog::Get().Register(id_, this)) {
            // Register() may have destroyed this object: its local strong
            // reference was the only one. Touch no member after this point.
            printf("[Item ctor] registration failed; this=%p may already be dead\n",
                   static_cast<void *>(this));
            return;
        }

        constructed_ = true;
        printf("[Item ctor] end     this=%p id=%d\n", static_cast<void *>(this), id_);
    }

    ~Item() override
    {
        --g_itemLive;
        printf("[~Item]     this=%p id=%d constructed_=%d\n",
               static_cast<void *>(this), id_, constructed_);

        if (!constructed_) {
            g_itemDestroyedInCtor = true;
            puts("[~Item]     !!! deleted before its constructor finished !!!");
        }
    }

    void OnFirstStrongRef(const void *objectId) override
    {
        printf("[Item OnFirstStrongRef] this=%p constructed_=%d triggering sptr=%p\n",
               static_cast<void *>(this), constructed_, objectId);
    }

    int GetId() const { return id_; }

private:
    int id_;
    bool constructed_;
};

Catalog &Catalog::Get()
{
    static Catalog instance;
    return instance;
}

bool Catalog::Register(int id, Item *item)
{
    // This local reference triggers OnFirstStrongRef() right now, i.e. while
    // the Item constructor is still running.
    sptr<Item> ref = item;

    auto result = items_.emplace(id, ref);
    if (!result.second) {
        printf("[Catalog] Register(id=%d) failed: id already exists (refs=%d)\n",
               id, ref->GetSptrRefCount());
        // Returning destroys `ref`. If the caller only has a raw `this`, this
        // is the last strong reference and the Item is deleted right here.
        return false;
    }

    printf("[Catalog] Register(id=%d) ok, size=%zu, refs=%d\n",
           id, items_.size(), ref->GetSptrRefCount());
    return true;
}

void Catalog::Clear()
{
    items_.clear();
}

void PrintHeader(const char *title)
{
    printf("\n\033[1;31m============================================================\033[0m\n");
    printf("\033[1;31m%s\033[0m\n", title);
    printf("\033[1;31m============================================================\033[0m\n");
}

void ReportChild(pid_t pid)
{
    int status = 0;
    if (waitpid(pid, &status, 0) < 0) {
        perror("waitpid");
        return;
    }

    if (WIFSIGNALED(status)) {
        int sig = WTERMSIG(status);
        printf("[parent] child killed by signal %d (%s) - reproduced\n",
               sig, strsignal(sig));
    } else if (WEXITSTATUS(status) != 0) {
        printf("[parent] child exited with code %d (ASan reports heap-use-after-free above) - reproduced\n",
               WEXITSTATUS(status));
    } else {
        puts("[parent] child exited with code 0 (it may just be lucky, but it is still UB)");
    }
}

void RunInChild(void (*body)())
{
    fflush(stdout);

    pid_t pid = fork();
    if (pid == 0) {
        body();
        _exit(0);
    }
    ReportChild(pid);
}

void TestRegistrationSucceeds()
{
    PrintHeader("test 1: Catalog auto-registration succeeds");
    g_itemDestroyedInCtor = false;

    {
        sptr<Item> item = new Item(10);
        printf("[main] item=%p refs=%d Catalog size=%zu Item live=%d\n",
               static_cast<void *>(item.GetRefPtr()), item->GetSptrRefCount(),
               Catalog::Get().Size(), g_itemLive);
    }

    printf("[main] after item is released: Catalog size=%zu Item live=%d (still owned)\n",
           Catalog::Get().Size(), g_itemLive);
}

void TestRegistrationFailsWithRawPointer()
{
    PrintHeader("test 2: duplicate id destroys the Item inside its ctor (raw pointer)");
    g_itemDestroyedInCtor = false;

    // id 10 is still owned by Catalog, so this registration fails.
    Item *rawItem = new Item(10);
    printf("[main] new returned %p (non-null), live=%d, destroyedInCtor=%d, Catalog size=%zu\n",
           static_cast<void *>(rawItem), g_itemLive, g_itemDestroyedInCtor,
           Catalog::Get().Size());
    puts("[main] rawItem looks valid but the object is gone; do not dereference it");
}

void ChildCreateDuplicateItem()
{
    sptr<Item> item = new Item(10);
    printf("[child] item.GetRefPtr()=%p (non-null), live=%d\n",
           static_cast<void *>(item.GetRefPtr()), g_itemLive);
    item->GetId();  // use-after-free
}

void TestRegistrationFailsWithSptr()
{
    PrintHeader("test 3: same failure, written as sptr<Item> item = new Item(10)");
    RunInChild(ChildCreateDuplicateItem);
}

}  // namespace

int main()
{
    setvbuf(stdout, nullptr, _IONBF, 0);

    puts("Catalog owns all Item objects; Item registers itself from its constructor.");
    puts("If that registration fails, the Item can be destroyed before its ctor returns.");

    TestRegistrationSucceeds();
    TestRegistrationFailsWithRawPointer();
    TestRegistrationFailsWithSptr();

    Catalog::Get().Clear();
    printf("[catalog] end, Item live=%d, Catalog size=%zu\n",
           g_itemLive, Catalog::Get().Size());
    return 0;
}
