#!/usr/bin/env node

var fs = require('fs');
var ranges = require('../ranges.js');

var composite = {};

Object.keys(ranges).forEach(function(range) {
    var sorted = JSON.parse(fs.readFileSync(__dirname + '/../expected/' + range + '-osm.json'));

    var merged = Object.keys(sorted).reduce(function(prev, locale) {
        for (var i = 0; i < sorted[locale].length; i++) {
            var glyph = sorted[locale][i];
            prev.hasOwnProperty(glyph.index) ? prev[glyph.index].count += glyph.count : prev[glyph.index] = glyph;
        }
        return prev;
    }, {});

    composite[range] = freqSort(merged);

    // Sort by range, then frequency, then Unicode index
    composite[range].sort(function(a, b) {
        if (a.count === b.count) {
            return a.index - b.index;
        } else {
            return b.count - a.count;
        }
    });

    var glyphs = {};

    composite[range].forEach(function(a,i) {
        glyphs[a.count + '-' + a.index] = String.fromCharCode(a.index);
    });

    fs.writeFileSync(__dirname + '/../expected/' + range + '-glyphs.json', JSON.stringify(glyphs, null, 2));
});

var sliced = {};
sliced['cjk'] = composite['cjk'].slice(0, 4096);

var cjk = sliced['cjk'].map(function(glyph) {
    return glyph.index;
});

sliced['cjk-extended'] = composite['cjk-extended'].filter(function(value) {
    return cjk.indexOf(value.index) === -1;
}); //.slice(0, 2048);

sliced['hangul'] = composite['hangul'].slice(0, 1024);

Object.keys(sliced).forEach(function(range) {
    var common = {};

    sliced[range].forEach(function(a,i) {
        common[a.index] = range + '-common';
    });

    fs.writeFileSync(__dirname + '/../expected/' + range + '-common.json', JSON.stringify(common, null, 2));
});

function freqSort(glyphs) {
    var frequency = [];
    for (var key in glyphs) frequency.push(glyphs[key]);
    frequency.sort(function(a, b) {
        return b.count - a.count;
    });
    return frequency;
}
