const video = document.getElementById('video');
const statusEl = document.getElementById('status');
const reloadBtn = document.getElementById('reloadBtn');
const sourceUrl = '/hls/stream.m3u8';
let hls;

function setStatus(text) {
  statusEl.textContent = text;
}

function destroyPlayer() {
  if (hls) {
    hls.destroy();
    hls = null;
  }
  video.removeAttribute('src');
  video.load();
}

function attachNative() {
  setStatus('Using native HLS');
  video.src = sourceUrl;
  video.play().catch(() => {});
}

function attachHlsJs() {
  setStatus('Connecting with hls.js');
  hls = new Hls({
    lowLatencyMode: true,
    backBufferLength: 30,
  });

  hls.loadSource(sourceUrl);
  hls.attachMedia(video);

  hls.on(Hls.Events.MANIFEST_PARSED, () => {
    setStatus('Live');
    video.play().catch(() => {});
  });

  hls.on(Hls.Events.ERROR, (_event, data) => {
    if (data.fatal) {
      setStatus(`Error: ${data.type}`);
      if (data.type === 'networkError') {
        hls.startLoad();
      } else if (data.type === 'mediaError') {
        hls.recoverMediaError();
      } else {
        destroyPlayer();
      }
    }
  });
}

function startPlayer() {
  destroyPlayer();

  if (video.canPlayType('application/vnd.apple.mpegurl')) {
    attachNative();
    return;
  }

  if (window.Hls && Hls.isSupported()) {
    attachHlsJs();
    return;
  }

  setStatus('HLS not supported in this browser');
}

reloadBtn.addEventListener('click', startPlayer);
startPlayer();
