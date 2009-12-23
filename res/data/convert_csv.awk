# awk script vor all lines
{
	nuc=$1;
	abun=$6;
	if (match(nuc,"n")) {
		nuc=-1;
		abun=100;
	};
	if (length(abun)==0) {
		abun=0.0;
	}
	bc_coh_re=$7;
	bc_coh_im=$8;
	bc_coh="";
	if (length(bc_coh_re)!=0) {
		bc_coh="re=\""bc_coh_re"\""
	};
	if (length(bc_coh_im)!=0) {
		bc_coh=bc_coh" im=\""bc_coh_im"\""
	};
	bc_inc_re=$9;
	bc_inc_im=$10;
	bc_inc="";
	if (length(bc_inc_re)!=0) {
		bc_inc="re=\""bc_inc_re"\""
	};
	if (length(bc_inc_im)!=0) {
		bc_inc=bc_inc" im=\""bc_inc_im"\""
	};
	bc="";
	if (length(bc_coh)!=0 || length(bc_inc)!=0) {
		bc="\t\t<neutron_scattering_length>\n"
		if (length(bc_coh)!=0) {
			bc=bc"\t\t\t<coherent "bc_coh"/>\n"
		};
		if (length(bc_inc)!=0) {
			bc=bc"\t\t\t<incoherent "bc_inc"/>\n"
		};
		bc=bc"\t\t</neutron_scattering_length>\n"
	}
	sigma_coh=$11;
	sigma_inc=$12;
	sigma_s=$13;
	sigma_abs=$14;
	sigma="";
	if (length(sigma_coh)!=0 || length(sigma_inc)!=0 || 
	    length(sigma_s)!=0 || length(sigma_abs)!=0) 
	{
		sigma="\t\t<neutron_scattering_cross_section>\n";
		if (length(sigma_coh)!=0) {
			sigma=sigma"\t\t\t<coherent re=\""sigma_coh"\"/>\n"
		}
		if (length(sigma_inc)!=0) {
			sigma=sigma"\t\t\t<incoherent re=\""sigma_inc"\"/>\n"
		}
		if (length(sigma_s)!=0) {
			sigma=sigma"\t\t\t<total val=\""sigma_s"\"/>\n"
		}
		if (length(sigma_abs)!=0) {
			sigma=sigma"\t\t\t<absorption val=\""sigma_abs"\"/>\n"
		}
		sigma=sigma"\t\t</neutron_scattering_cross_section>\n";
	}
	coefs="";
	if (nuc==-1) {
		# get the anomalous x-ray scattering coefficients from file
		fn=xas_dir $3".dat"
		err=system("ls "fn" > /dev/null 2>&1");
		if (!err) { # file exists
			coefs="\t\t<xray_scattering_anomalous_coefficients>\n"
			header=1;
			while (getline line < fn) {
				if (header) {
					if (match(line, /^Scattering factors.*/)) {
						header=0;
					}
				} else if (match(line, /[0-9]\.[0-9].+[0-9]\.[0-9].+[0-9]\.[0-9]/)){
					split(line, arr, " ");
					energy=arr[1];
					fp=arr[2];
					fpp=arr[3];
					coefs=coefs"\t\t\t<ev val=\""energy"\" fp=\""fp"\" fpp=\""fpp"\"/>\n"
				}
			}
			coefs=coefs"\t\t</xray_scattering_anomalous_coefficients>\n"
		}
	}
	
	record="\t<chemical_element symbol=\""$3"\">\n\
		<name val=\""$4"\"/>\n\
		<abundance val=\""abun"\"/>\n\
		<atomic_weight val=\""$2"\"/>\n\
		<nucleons val=\""nuc"\"/>\n\
		<electrons val=\""$5"\"/>\n\
"bc"\
"sigma"\
"coefs"\
	</chemical_element>";
	if (nuc==-1) {
		header="<?xml version=\"1.0\" encoding=\"utf-8\"?>\n\
<!DOCTYPE chemical_element_list SYSTEM \"chemical_elements.dtd\">\n\
<chemical_element_list>"
		cmd="echo '"header"' > "$3".xml";
		system(cmd);
	}
	cmd="echo '"record"' >> "$3".xml";
#	print cmd
	system(cmd);
}

