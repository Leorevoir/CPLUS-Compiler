def greet(name) -> string
{
    const message = "Hello, " + name + "!";
    const newline = '\n';

    return message + newline;
}

def main() -> int
{
    print(greet("World"));
    return 0;
}
