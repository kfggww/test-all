#include <cstdio>
#include <set>
#include <algorithm>
#include <functional>

using namespace std;

struct Rect {
    int area;
};

bool operator<(const Rect &a, const Rect &b) { return a.area < b.area; }

int main(int argc, char **argv) {

    set<Rect> s1;
    Rect r1 = {200};
    Rect r2 = {100};

    s1.insert(r1);
    s1.insert(r2);
    printf("s1.size=%ld\n", s1.size());
    s1.insert(r2);
    printf("s1.size=%ld\n", s1.size());

    printf("\n\nset1: \n");
    std::for_each(s1.begin(), s1.end(),
                  [](const Rect &r) { printf("area=%d\n", r.area); });

    return 0;
}