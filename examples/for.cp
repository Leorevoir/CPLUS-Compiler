def main() -> int
{
    for i = 0; i < 10; ++i {
        print(i);
    }
    for (i = 0; i < 10; ++i) {
        print(i);
    }
    foreach c in "Hello C+" {
        print(c);
    }
    foreach (c in "Hello C+") {
        print(c);
    }
    return 0;
}
