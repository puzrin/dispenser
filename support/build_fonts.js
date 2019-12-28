#! /usr/bin/env node

const path = require('path');
const { execSync } = require('child_process');
const fs = require('fs');

const convertor = '../node_modules/.bin/lv_font_conv';
const roboto = '../node_modules/roboto-fontface/fonts/roboto/Roboto-Medium.woff';
const icons = 'icons.ttf';
const outdir = '../src/fonts';

function fix_include_path(file) {
  file = path.join(__dirname, file);
  fs.writeFileSync(
    file,
    fs.readFileSync(file, 'utf8').replace('lvgl/lvgl.h', 'lvgl.h')
  );
}

let out_file;
let bpp = 3;
let compression = '';

if (process.argv[2] == 'nocompress') {
  bpp = 4;
  compression = '--no-compress';
}


out_file = path.join(outdir, 'my_font_roboto_12.c');
console.log(`Build ${path.join(__dirname, out_file)}`);
execSync(`${convertor} --bpp ${bpp} ${compression} --size 12 --lcd --font ${roboto} --range 0x20-0x7F --symbols ³ --font ${icons} --range 0xE006 --format lvgl --force-fast-kern-format -o ${out_file}`, { stdio: 'inherit', cwd: __dirname });
fix_include_path(out_file);

out_file = path.join(outdir, 'my_font_roboto_14.c');
console.log(`Build ${path.join(__dirname, out_file)}`);
execSync(`${convertor} --bpp ${bpp} ${compression} --size 14 --lcd --font ${roboto} --range 0x20-0x7F --symbols ³ --font ${icons} --range 0xE006 --format lvgl --force-fast-kern-format -o ${out_file}`, { stdio: 'inherit', cwd: __dirname });
fix_include_path(out_file);

out_file = path.join(outdir, 'my_font_roboto_num_18.c');
console.log(`Build ${path.join(__dirname, out_file)}`);
execSync(`${convertor} --bpp ${bpp} ${compression} --size 18 --lcd --font ${roboto} --symbols 0123456789. --font ${icons} --range 0xE003,0xE004 --range '0xE005=>0x78' --format lvgl --force-fast-kern-format -o ${out_file}`, { stdio: 'inherit', cwd: __dirname });
fix_include_path(out_file);

out_file = path.join(outdir, 'my_font_icons_18.c');
console.log(`Build ${path.join(__dirname, out_file)}`);
execSync(`${convertor} --bpp ${bpp} ${compression} --size 18 --lcd --font ${icons} --range 0xE000,0xE001,0xE002 --format lvgl --force-fast-kern-format -o ${out_file}`, { stdio: 'inherit', cwd: __dirname });
fix_include_path(out_file);
