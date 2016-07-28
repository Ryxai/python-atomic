import unittest

import atomic


class TestAtomicReference(unittest.TestCase):
    def test_init(self):
        o = atomic.Reference()
        self.assertIs(o.get(), None)

        d = {}
        o = atomic.Reference(d)
        self.assertIs(o.get(), d)

    def test_set(self):
        d = {}
        o = atomic.Reference()
        o.set(d)
        self.assertIs(o.get(), d)

    def test_get_and_set(self):
        d1 = {}
        d2 = {}
        o = atomic.Reference(d1)
        ret = o.get_and_set(d2)
        self.assertEqual(ret, d1)
        self.assertEqual(o.get(), d2)

    def test_compare_and_set(self):
        d1 = {}
        d2 = {}
        d3 = {}
        o = atomic.Reference(d1)

        ret = o.compare_and_set(d1, d2)
        self.assertTrue(ret)
        self.assertIs(o.get(), d2)

        ret = o.compare_and_set(d1, d3)
        self.assertFalse(ret)
        self.assertIs(o.get(), d2)

    def test_reference_cycle(self):
        o = atomic.Reference()
        o.set(o)
        del o


if __name__ == '__main__':
    unittest.main()
