---
tags:
  - command
---

# /aaspend

## Syntax

<!--cmd-syntax-start-->
```eqcommand
/aaspend [option] [AA Name] [setting]
```
<!--cmd-syntax-end-->

## Description

<!--cmd-desc-start-->
Allows changing .ini settings for aaspend via the commandline.
<!--cmd-desc-end-->

## Options

| Option | Description |
|--------|-------------|
| `(no option)` | lists command syntax |
| `status` | Shows current status |
| `add <AA Name> level` | Adds AA Name to ini file, will not purchase past level. Use M to max out available levels. |
| `del <AA Name>` | Deletes AA Name from ini file |
| `debug {On|Off}` | turn on or off debug. |
| `buy <AA Name>` | Buys a particular AA. Ignores bank setting. |
| `brute [on|off|now]` | Buys first available ability on ding, or now if specified. Default |
| `auto [on|off|now]` | Autospends AA's based on ini on ding, or now if specified. Default |
| `bank <#>` | Keeps this many AA points banked before auto/brute spending. Default |
| `load` | Reload ini file |
| `save` | Save current settings to the ini file |
| `bonus [on|off|now]` | only buy AA that aren't autogranted. A powerleveler's best friend! |
| `order 12345` | Set the order of the tabs to be spent when brute force is on. 1=general,2=archetype,3=class,4=special,5=focus |
| `MercBrute [on|off|now]` | Same as Brute for the player, but is used for the mercenary. |
| `MercAuto [on|off|now]` | Same as Auto for the player, but is used for the mercenary. |
| `MercAdd <aa name>` | Add an AA to the list of AA's to purchase with MercAuto. |
| `MercDel <aa name>` | Remove an AA from the list of AA's to purchase with MercAuto. |
| `MercBuy <aa name>` | Attempt to buy a specific Merc AA. Useful if you know what AA you want, but don't want to find it in the AA window. |
| `MercOrder [auto]` | Specify the order to buy MercAA's for MercAuto and MercBrute modes. General = 1, Tank = 2, Healer = 3, Melee DPS = 4, Caster DPS = 5. Ex: /autospend MercOrder 32451. If "auto" is specified, it will prefer to purchase AA's based on the currently active mercenary. |

## Examples

`/aaspend order 32541`

Brute force mode would buy all AAs in this order Class, Archetype, Focus, Special, General