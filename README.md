# MQ2AASpend

This plugin assists with AA spending.

## Commands

```txt
/aaspend - Lists command syntax
/aaspend status - Shows current status
/aaspend add "AA Name" level - Adds AA Name to ini file, will not purchase past level. Use M to max out available levels.
/aaspend del "AA Name" - Deletes AA Name from ini file
/aaspend debug On|Off - turn on or off debug.
/aaspend buy "AA Name" - Buys a particular AA. Ignores bank setting.
/aaspend brute on|off|now - Buys first available ability on ding, or now if specified. Default
/aaspend auto on|off|now - Autospends AA's based on ini on ding, or now if specified. Default
/aaspend bank # - Keeps this many AA points banked before auto/brute spending. Default
/aaspend load - Reload ini file
/aaspend save - Save current settings to the ini file
/aaspend bonus on - only buy AA that aren't autogranted. A powerleveler's best friend!
/aaspend order 12345 - Set the order of the tabs to be spent when brute force is on. 1=general,2=archetype,3=class,4=special,5=focus
```

`/aaspend order 32541` - Brute force mode would buy all AAs in this order Class, Archetype, Focus, Special, General