(() => {
  const host = window.chrome.webview;
  const bridge = window.earthDesktop = {
    captured: false,
    capture: () => host.postMessage('earth:capture'),
    release: () => host.postMessage('earth:release')
  };
  host.addEventListener('message', event => {
    const message = event.data;
    if (message?.type === 'earth:mouse') {
      if (bridge.captured && Number.isFinite(message.dx) && Number.isFinite(message.dy))
        window.dispatchEvent(new CustomEvent('earth-desktop-move', {detail:message}));
    } else if (message?.type === 'earth:capture') {
      bridge.captured = message.captured === true;
      document.documentElement.classList.toggle('desktop-mouse-captured', bridge.captured);
      window.dispatchEvent(new Event('earth-desktop-capture'));
    }
  });
  document.addEventListener('DOMContentLoaded', () => {
    const style = document.createElement('style');
    style.textContent = '.desktop-mouse-captured,.desktop-mouse-captured *{cursor:none!important}';
    document.head.append(style);
  });
})();
