"use strict";

// Insert header
fetch("./header.html")
	.then(response => response.text())
	.then(headerCode => document.body.insertAdjacentHTML("afterbegin", headerCode));

// Insert footer
fetch("./footer.html")
	.then(response => response.text())
	.then(footerCode => document.body.insertAdjacentHTML("beforeend", footerCode));

// Intersection observer for animations
const observer = new IntersectionObserver(entries => {
	entries.forEach(entry => {
		if (entry.isIntersecting) {
			entry.target.classList.add("fade-in");
		}
	});
});
document.querySelectorAll("h1").forEach(h1 => observer.observe(h1));
