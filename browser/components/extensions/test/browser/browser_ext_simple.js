add_task(function* test_simple() {
  let extensionData = {
    manifest: {
      "name": "Simple extension test",
      "version": "1.0",
      "manifest_version": 2,
      "description": ""
    }
  };

  let extension = ExtensionTestUtils.loadExtension(extensionData);
  info("load complete");
  yield extension.startup();
  info("startup complete");
  yield extension.unload();
  info("extension unloaded successfully");
});

add_task(function* test_background() {
  function backgroundScript() {
    browser.test.log("running background script");

    browser.test.onMessage.addListener((x, y) => {
      browser.test.assertEq(x, 10, "x is 10");
      browser.test.assertEq(y, 20, "y is 20");

      browser.test.notifyPass("background test passed");
    });

    browser.test.sendMessage("running", 1);
  }

  let extensionData = {
    background: "(" + backgroundScript.toString() + ")()",
    manifest: {
      "name": "Simple extension test",
      "version": "1.0",
      "manifest_version": 2,
      "description": ""
    }
  };

  let extension = ExtensionTestUtils.loadExtension(extensionData);
  info("load complete");
  let [, x] = yield Promise.all([extension.startup(), extension.awaitMessage("running")]);
  is(x, 1, "got correct value from extension");
  info("startup complete");
  extension.sendMessage(10, 20);
  yield extension.awaitFinish();
  info("test complete");
  yield extension.unload();
  info("extension unloaded successfully");
});
