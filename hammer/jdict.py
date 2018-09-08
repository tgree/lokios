#!/usr/bin/python
import requests


def as_jdict(self):
    return jdict(self.json())
requests.Response.jdict = as_jdict


class jdict(object):
    def __init__(self, d):
        assert type(d) is dict

        super(jdict, self).__init__()
        super(jdict, self).__setattr__('_data',{})
        for k,v in d.items():
            assert type(k) in [str, unicode]
            if type(v) is dict:
                self._data[k] = jdict(v)
            elif type(v) is list:
                self._data[k] = [jdict(_v)
                                 if type(_v) is dict else _v for _v in v]
            else:
                self._data[k] = v

    def __getattr__(self, name):
        return self._data[name]

    def __setattr__(self, name, value):
        raise Exception('jdict is read-only')

    def __getitem__(self, key):
        return self._data[key]

    def __setitem__(self, key, value):
        raise Exception('jdict is read-only')

    def set_attr(self, key, value):
        self._data[key] = value

    def has_attr(self, key):
        return key in self._data


if __name__ == '__main__':
    d = jdict({'abra' : 1,
               'cadabra' : {'text' : '2222', 'string' : 'terry'},
               'list' : [{'txet' : 'yrret'}, 3333, 'text'],
               '1' : 'one',
               })
    assert d.abra == 1
    assert d.cadabra.text == '2222'
    assert d.cadabra.string == 'terry'
    assert d.list[0].txet == 'yrret'
    assert d.list[1] == 3333
    assert d.list[2] == 'text'
    assert d['1'] == 'one'
