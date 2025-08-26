def main() -> int
{
    /**
    * @brief variable assignments
    *
    * if you directly specify the value, the type is deduced automatically
    * if you want to specify the type, use `:` no matter of space
    */
    my_number = 1;
    my_integer:int = 2;
    my_float : float = 3.0;

    if (my_number < my_integer) {
        print("my_number is less than my_integer");
    } else {
        return 84;
    }

    str = "Hellloo C+";

    foreach (c in str) {
        print(c);
    }

    for (i = 0; i < 10; ++i) {
        print(i);
    }

    case (my_integer) {
        1: print("one");
        2: print("two");
        3: print("three");
        default: print("other");
    }

    return 0;
}
