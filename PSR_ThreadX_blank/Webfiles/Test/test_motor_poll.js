const http = require('http');
const url = new URL(process.env.ENDPOINT || 'http://192.168.1.11/MOTOR_STATUS');
const INTERVAL = 200;
const WINDOW_MS = 60_000;

let samples = [];

function pollOnce() {
    const opts = {
        hostname: url.hostname,
        port: url.port || 80,
        path: url.pathname + url.search,
        method: 'GET',
        timeout: 2000
    };
    const req = http.request(opts, res => {
        let body = '';
        res.setEncoding('utf8');
        res.on('data', chunk => body += chunk);
        res.on('end', () => {
            const ts = Date.now();
            const v = parseFloat(body);
            if (!Number.isNaN(v)) {
                samples.push({t: ts, v});
            }
            // trim window
            const cutoff = ts - WINDOW_MS;
            while (samples.length && samples[0].t < cutoff) samples.shift();

            // output last value and simple stats
            const last = samples.length ? samples[samples.length-1].v : null;
            const count = samples.length;
            const min = count ? Math.min(...samples.map(s=>s.v)) : null;
            const max = count ? Math.max(...samples.map(s=>s.v)) : null;
            process.stdout.write(`${new Date(ts).toISOString()} val=${body.trim()} parsed=${Number.isFinite(last)?last:'NaN'} samples=${count} min=${min} max=${max}\r\n`);
        });
    });
    req.on('error', (e) => {
        console.error('request error', e.message);
    });
    req.on('timeout', () => { req.abort(); });
    req.end();
}

console.log('Polling', url.toString(), 'every', INTERVAL, 'ms. Press Ctrl+C to stop.');
setInterval(pollOnce, INTERVAL);
pollOnce();