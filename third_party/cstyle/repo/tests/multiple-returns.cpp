namespace _TestNS
{

// This is a test class.
//
// class _Test1 { ... };
//
class _Test1
{
public:
    _Test1(void) { return; }
    ~_Test1(void) { return; }

    static int Mul(int bar) { return (bar * 3); }

    static void Baz(int bar)
    {
        if (bar)
            sMoof += bar;
        else
            sMoof += Mul(bar);

        return;
    }

    int Foo(int bar) {
        if (bar)
            return (1);

        if (bar % 2) {
            return (1 + bar);
        }

        return 0;
    }

    bool Bar(int bar) const
    {
        if (bar % 2 == 0)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

private:
    static int sMoof;
};

}; // namespace _TestNS
