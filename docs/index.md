---
tags:
  - plugin
resource_link: "https://www.redguides.com/community/resources/mq2aaspend.78/"
support_link: "https://www.redguides.com/community/threads/mq2aaspend.24697/"
repository: "https://github.com/RedGuides/MQ2AASpend"
config: "Server_Character.ini"
authors: "Sym, ChatWithThisName, eqmule, Sic, Knightly, brainiac"
tagline: "This plugin assists with AA spending."
---

# MQ2AASpend
<!--desc-start-->
Allows you to spend your AA's more wisely. Whether you forget to spend them or you want to buy them in a specific order, there's an option here for you. There's even handling to avoid AA that will be autogranted.
<!--desc-end-->

## Commands

<a href="cmd-aaspend/">
{% 
  include-markdown "projects/mq2aaspend/cmd-aaspend.md" 
  start="<!--cmd-syntax-start-->" 
  end="<!--cmd-syntax-end-->"
%}
</a>
:    {% include-markdown "projects/mq2aaspend/cmd-aaspend.md" 
        start="<!--cmd-desc-start-->" 
        end="<!--cmd-desc-end-->" 
        trailing-newlines=false 
     %} {{ readMore('projects/mq2aaspend/cmd-aaspend.md') }}

## Settings

In your Server_Character.ini, you'll find settings:
```ini
[MQ2AASpend_Settings]
AutoSpend=1
BruteForce=0
BankPoints=15

[MQ2AASpend_AAList]
0=Mystical Attuning|8
1=Combat Agility|10
2=Combat Stability|10
3=Natural Durability|M
4=Physical Enhancement|M
```

The command page has descriptions of these options.
