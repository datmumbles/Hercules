#!/usr/bin/perl
use strict;
use warnings;

sub prettifyscript ($) {
	my ($orig) = @_;
	$orig =~ s/^[\s\t]*//; $orig =~ s/[\s\t]*$//;
	return '' unless $orig =~ /[^\s\t]/;
	my ($p, $script) = ($orig, '');
	my ($curly, $lines, $comment) = (2, 0, 0);
	my ($linebreak, $needindent) = (0, 0);
	while ($p =~ /[^\s\t]/) {
		$linebreak = 0;
		if ($comment && $p =~ s|^\s*\*/\s*||) {
			$comment = 0;
			next;
		} elsif ($p =~ s/^\s*({)\s*//) {
			$curly++ unless $comment;
			$comment++ if $comment;
			$script .= " ";
			$linebreak = 1;
			$lines++;
		} elsif ($p =~ s/^\s*(})\s*//) {
			$curly-- unless $comment;
			$comment-- if $comment - 1 > 0;
			$linebreak = 1;
			$lines++;
		} elsif ($p =~ s/^\s*(;)\s*//) {
			if ($p && (!$comment || $p !~ m|^[\s\t]*(?:\*/)[\s\t]*$|)) {
				$linebreak = 1;
				$lines++
			}
		} elsif ($p =~ s/^("[^"]*")//) {
		} elsif ($p =~ s|^\s*/\*\s*||) {
			$comment = 1;
			next;
		} elsif ($p !~ s/^(.)//) {
			last;
		}
		$script .= "\t" x $curly if $needindent;
		$script .= "//" . ("\t" x ($comment-1)) if ($comment && ($needindent || $script eq ''));
		$script .= "$1";
		if ($linebreak) {
			$script .= "\n";
			$needindent = 1;
		} else {
			$needindent = 0;
		}
	}
	if ($curly != 2) {
		printf STDERR "Parse error, curly braces count ". ($curly-2) .". returning unmodified script:\n$orig\n\n";
		return $orig;
	}
	if ($lines) {
		$script = "\n\t\t$script\n\t";
	} else {
		$script = " $script ";
	}
	return $script;
}

print <<"EOF";
item_db: (

/******************************************************************************
{
	// =================== Mandatory fields ===============================
	Id: ID                        (int)
	AegisName: "Aegis_Name"       (string)
	Name: "Item Name"             (string)
	Type: Item Type               (int)
	// =================== Optional fields ================================
	Buy: Buy Price                (int, defaults to Sell * 2)
	Sell: Sell Price              (int, defaults to Buy / 2)
	Weight: Item Weight           (int, defaults to 0)
	Atk: Attack                   (int, defaults to 0)
	Matk: Magical Attack          (int, defaults to 0, ignored in pre-re)
	Def: Defense                  (int, defaults to 0)
	Range: Attack Range           (int, defaults to 0)
	Slots: Slots                  (int, defaults to 0)
	Job: Job mask                 (int, defaults to all jobs = 0xFFFFFFFF)
	Upper: Upper mask             (int, defaults to any = 0x3f)
	Gender: Gender                (int, defaults to both = 2)
	Loc: Equip location           (int, required value for equipment)
	WeaponLv: Weapon Level        (int, defaults to 0)
	EquipLv: Equip required level (int, defaults to 0)
	EquipLv: [min, max]           (alternative syntax with min / max level)
	Refine: Refineable            (boolean, defaults to true)
	View: View ID                 (int, defaults to 0)
	Script: <"
		Script
		(it can be multi-line)
	">
	OnEquipScript: <" OnEquip Script (can also be multi-line) ">
	OnUnequipScript: <" OnUnequip Script (can also be multi-line) ">
},
******************************************************************************/

EOF

while (<>) {
	chomp $_;
#	ID,AegisName,Name,Type,Buy,Sell,Weight,ATK,DEF,Range,Slots,Job,Upper,Gender,Loc,wLV,eLV,Refineable,View,{ Script },{ OnEquip_Script },{ OnUnequip_Script }
	if( $_ =~ qr/^
		(?<prefix>(?:\/\/[^0-9]*)?)
		(?<ID>[0-9]+),
		(?<AegisName>[^,]+),
		(?<Name>[^,]+),
		(?<Type>[0-9]+),
		(?<Buy>[0-9]*),
		(?<Sell>[0-9]*),
		(?<Weight>[0-9]*),
		(?<ATK>[0-9]*)(?<hasmatk>:(?<MATK>[0-9]*))?,
		(?<DEF>[0-9]*),
		(?<Range>[0-9]*),
		(?<Slots>[0-9]*),
		(?<Job>[x0-9A-F]*),
		(?<Upper>[0-9]*),
		(?<Gender>[0-9]*),
		(?<Loc>[0-9]*),
		(?<wLV>[0-9]*),
		(?<eLV>[0-9]*)(?<hasmaxlv>:(?<eLVmax>[0-9]*))?,
		(?<Refineable>[0-9]*),
		(?<View>[0-9]*),
		{(?<Script>.*)},
		{(?<OnEquip>.*)},
		{(?<OnUnequip>.*)}
	/x ) {
		my %cols = map { $_ => $+{$_} } keys %+;
		print "/" . ('*' x 78) . "\n$cols{prefix}\n" if $cols{prefix};
		print "{\n";
		print "\tId: $cols{ID}\n";
		print "\tAegisName: \"$cols{AegisName}\"\n";
		print "\tName: \"$cols{Name}\"\n";
		print "\tType: $cols{Type}\n";
		print "\tBuy: $cols{Buy}\n" if $cols{Buy} || $cols{Buy} eq '0';
		print "\tSell: $cols{Sell}\n" if $cols{Sell} || $cols{Sell} eq '0';
		print "\tWeight: $cols{Weight}\n" if $cols{Weight};
		print "\tAtk: $cols{ATK}\n" if $cols{ATK};
		print "\tMatk: $cols{MATK}\n" if $cols{MATK};
		print "\tDef: $cols{DEF}\n" if $cols{DEF};
		print "\tRange: $cols{Range}\n" if $cols{Range};
		print "\tSlots: $cols{Slots}\n" if $cols{Slots};
		$cols{Job} = '0xFFFFFFFF' unless $cols{Job};
		print "\tJob: $cols{Job}\n" unless $cols{Job} =~ /0xFFFFFFFF/i;
		print "\tUpper: $cols{Upper}\n" if $cols{Upper} && (($cols{hasmatk} && $cols{Upper} != 0x3f) || (!$cols{hasmatk} && $cols{Upper} != 7));
		$cols{Gender} = '2' unless $cols{Gender};
		print "\tGender: $cols{Gender}\n" unless $cols{Gender} eq '2';
		print "\tLoc: $cols{Loc}\n" if $cols{Loc};
		print "\tWeaponLv: $cols{wLV}\n" if $cols{wLV};
		if ($cols{hasmaxlv} and $cols{eLVmax}) {
			$cols{eLV} = 0 unless $cols{eLV};
			print "\tEquipLv: [$cols{eLV}, $cols{eLVmax}]\n";
		} else {
			print "\tEquipLv: $cols{eLV}\n" if $cols{eLV};
		}
		print "\tRefine: false\n" unless $cols{Refineable};
		print "\tView: $cols{View}\n" if $cols{View};
		$cols{Script} = prettifyscript($cols{Script});
		print "\tScript: <\"$cols{Script}\">\n" if $cols{Script};
		$cols{OnEquip} = prettifyscript($cols{OnEquip});
		print "\tOnEquipScript: <\"$cols{OnEquip}\">\n" if $cols{OnEquip};
		$cols{OnUnequip} = prettifyscript($cols{OnUnequip});
		print "\tOnUnequipScript: <\"$cols{OnUnequip}\">\n" if $cols{OnUnequip};
		print "},\n";
		print '*' x 78 ."/\n" if $cols{prefix};
	} elsif( $_ =~ /^\/\/(.*)$/ ) {
		print "// $1\n";
	} elsif( $_ !~ /^\s*$/ ) {
		print "// Error parsing: $_\n";
	}

}
print ")\n";
