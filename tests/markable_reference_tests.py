import unittest

import atomic

class TestAtomicMarkableReference(unittest.TestCase):
    def test_init(self):
        o = atomic.MarkableReference()
        self.assertFalse(o.is_marked())
        self.assertIs(o.get_reference(), None)
        
        d = {}
        o = atomic.MarkableReference(d)
        self.assertFalse(o.is_marked())
        self.assertIs(o.get_reference(), d)
        
        d = {}
        m = True
        o = atomic.MarkableReference(d, m)
        self.assertTrue(o.is_marked())
        self.assertIs(o.get_reference(), d)
        
    def test_set(self):
        d = {}
        m = True
        o = atomic.MarkableReference()
        o.set(d, m)
        self.assertTrue(o.is_marked())
        self.assertIs(o.get_reference(), d)
    
    def test_get(self):
        d = {}
        m = True
        o = atomic.MarkableReference(d, m)
        (ret, mark) = o.get()
        self.assertIs(ret, d)
        self.assertIs(ret, o.get_reference())
        self.assertTrue(mark)
        self.assertTrue(o.is_marked())
        
    def test_compare_and_set(self):
        d1 = {}
        d2 = {}
        d3 = {}
        m1 = True
        m2 = False
        m3 = True
        o = atomic.MarkableReference(d1, m1)
        
        ret = o.compare_and_set(d1, d2, m1, m2)
        self.assertTrue(ret)
        self.assertFalse(o.is_marked())
        self.assertIs(o.get_reference(), d2)
        
        ret = o.compare_and_set(d1, d3, m1, m3)
        self.assertFalse(ret)
        self.assertFalse(o.is_marked())
        self.assertIs(o.get_reference(), d2)
        self.assertIsNot(o.get_reference(), d3)
        
    def test_weak_compare_and_set(self):
        d1 = {}
        d2 = {}
        d3 = {}
        m1 = True
        m2 = False
        m3 = True
        o = atomic.MarkableReference(d1, m1)
        
        ret = o.weak_compare_and_set(d1, d2, m1, m2)
        self.assertTrue(ret)
        #self.assertFalse(o.is_marked())
        self.assertIs(o.get_reference(), d2)
        
        ret = o.weak_compare_and_set(d1, d3, m1, m3)
        self.assertFalse(ret)
        self.assertFalse(o.is_marked())
        self.assertIs(o.get_reference(), d2)
        self.assertIsNot(o.get_reference(), d3)
    
    def test_attempt_mark(self):
        d = {}
        o = atomic.MarkableReference(d)
        self.assertFalse(o.is_marked())
        o.attempt_mark(False, True)
        self.assertTrue(o.is_marked())
        
    def test_reference_cycle(self):
        o = atomic.MarkableReference()
        o.set(o, True)
        del o

if __name__ == '__main__':
    unittest.main()