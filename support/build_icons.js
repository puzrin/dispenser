#! /usr/bin/env node

const join = require('path').join;
const SVGIcons2SVGFontStream = require('svgicons2svgfont');
const fs = require('fs');
const fontStream = new SVGIcons2SVGFontStream({
  fontName: 'dispicons',
  descent: 250,

});

// Setting the font destination
fontStream.pipe(fs.createWriteStream(join(__dirname, 'icons.svg')))
  .on('error', err => console.log(err));

let glyph;

glyph = fs.createReadStream(join(__dirname, 'design_src/icon_dose.svg'));
glyph.metadata = { unicode: ['\uE000'], name: 'icon0' };
fontStream.write(glyph);

glyph = fs.createReadStream(join(__dirname, 'design_src/icon_flow.svg'));
glyph.metadata = { unicode: ['\uE001'], name: 'icon1' };
fontStream.write(glyph);

glyph = fs.createReadStream(join(__dirname, 'design_src/icon_settings.svg'));
glyph.metadata = { unicode: ['\uE002'], name: 'icon2' };
fontStream.write(glyph);

glyph = fs.createReadStream(join(__dirname, 'design_src/icon_left.svg'));
glyph.metadata = { unicode: ['\uE003'], name: 'icon3' };
fontStream.write(glyph);

glyph = fs.createReadStream(join(__dirname, 'design_src/icon_right.svg'));
glyph.metadata = { unicode: ['\uE004'], name: 'icon4' };
fontStream.write(glyph);

glyph = fs.createReadStream(join(__dirname, 'design_src/icon_x.svg'));
glyph.metadata = { unicode: ['\uE005'], name: 'icon5' };
fontStream.write(glyph);

glyph = fs.createReadStream(join(__dirname, 'design_src/icon_arrows.svg'));
glyph.metadata = { unicode: ['\uE006'], name: 'icon6' };
fontStream.write(glyph);

fontStream.end();
