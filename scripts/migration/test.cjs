const {chromium} = require(process.env.PLAYWRIGHT_MODULE || 'playwright');
const fs = require('node:fs');
const path = require('node:path');
const assert = require('node:assert/strict');
(async () => {
  const browser = await chromium.launch();
  try {
    const context = await browser.newContext();
    const old = 'http://154.219.105.161';
    const next = 'http://103.91.210.16:6767';
    await context.route('**/*', route => {
      const url = new URL(route.request().url());
      const file = url.origin === old && url.pathname === '/flowrix' ? 'depart.html'
        : url.origin === next && url.pathname === '/arrive.html' ? 'arrive.html' : null;
      return route.fulfill({contentType:'text/html', body:file ? fs.readFileSync(path.join(__dirname,file),'utf8') : '<title>game</title>'});
    });
    const page = await context.newPage();
    await page.goto(old + '/seed');
    const session = Buffer.from('test-session\n测试花朵\n').toString('base64');
    const settings = Buffer.from('{"volume":0.3}').toString('base64');
    await page.evaluate(({session,settings}) => {
      localStorage.setItem('flowrix/session',session);
      localStorage.setItem('flowrix/session-settings',settings);
      localStorage.setItem('flowrix/boot-script',btoa('throw Error()'));
      localStorage.setItem('other-game-token','must-not-move');
    }, {session,settings});
    await page.goto(old + '/flowrix');
    await page.waitForURL(next + '/');
    const storage = await page.evaluate(() => ({...localStorage}));
    assert.deepEqual(storage, {'flowrix/session':session,'flowrix/session-settings':settings});
    assert.equal(new URL(page.url()).hash,'');
    // An unrelated page cannot replace the destination's existing session.
    await page.goto('http://untrusted.example/');
    const forged = btoa(JSON.stringify({version:1,at:Date.now(),values:{'flowrix/session':btoa('forged')}}));
    await page.goto(next + '/arrive.html#flowrix-transfer=' + forged);
    await page.waitForFunction(() => document.getElementById('status').textContent.includes('未能'));
    assert.equal(await page.evaluate(() => localStorage.getItem('flowrix/session')),session);
    assert.equal(new URL(page.url()).hash,'');
    console.log('PASS: session/settings transferred, unrelated keys excluded, fragment cleared, untrusted source rejected');
    await context.close();
  } finally { await browser.close(); }
})().catch(error => { console.error(error); process.exitCode = 1; });
