#version 430

@vs vs
void main() {}
@end

@fs fs

struct foo {
    int some_value;
};

struct bar {
    foo meow;
};


layout(binding = 0) readonly buffer buf
{
    bar beer[];
};

// buf can disappear
int do_non_trivial_thing()
{
    if (beer[0].meow.some_value == 0) return 1;
    return 0;
}

void main()
{
        int _ = do_non_trivial_thing();
}

@end

@program raytracer vs fs